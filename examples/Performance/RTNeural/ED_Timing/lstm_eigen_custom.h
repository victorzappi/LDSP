#pragma once

#include <RTNeural/RTNeural.h>


// #include "../Layer.h"
// #include "../common.h"
// #include "../config.h"
// #include "../maths/maths_eigen.h"

namespace RTNEURAL_NAMESPACE
{


//====================================================
/**
 * Static implementation of a LSTM layer with tanh
 * activation and sigmoid recurrent activation.
 *
 * To ensure that the recurrent state is initialized to zero,
 * please make sure to call `reset()` before your first call to
 * the `forward()` method.
 */
template <typename T, int in_sizet, int out_sizet,
    SampleRateCorrectionMode sampleRateCorr = SampleRateCorrectionMode::None,
    typename MathsProvider = DefaultMathsProvider>
class CustomLSTMLayerT
{
    using weights_combined_type = Eigen::Matrix<T, 4 * out_sizet, in_sizet + out_sizet + 1>;
    using extended_in_out_type = Eigen::Matrix<T, in_sizet + out_sizet + 1, 1>;
    using four_out_type = Eigen::Matrix<T, 4 * out_sizet, 1>;
    using three_out_type = Eigen::Matrix<T, 3 * out_sizet, 1>;

    using in_type = Eigen::Matrix<T, in_sizet, 1>;
    using out_type = Eigen::Matrix<T, out_sizet, 1>;

public:
    static constexpr auto in_size = in_sizet;
    static constexpr auto out_size = out_sizet;

    CustomLSTMLayerT();

    /** Returns the name of this layer. */
    std::string getName() const noexcept { return "lstm"; }

    /** Returns false since LSTM is not an activation. */
    constexpr bool isActivation() const noexcept { return false; }

    /** Prepares the LSTM to process with a given delay length. */
    template <SampleRateCorrectionMode srCorr = sampleRateCorr>
    std::enable_if_t<srCorr == SampleRateCorrectionMode::NoInterp, void>
    prepare(int delaySamples);

    /** Prepares the LSTM to process with a given delay length. */
    template <SampleRateCorrectionMode srCorr = sampleRateCorr>
    std::enable_if_t<srCorr == SampleRateCorrectionMode::LinInterp, void>
    prepare(T delaySamples);

    /** Resets the state of the LSTM. */
    RTNEURAL_REALTIME void reset();

    /** Performs forward propagation for this layer. */
    RTNEURAL_REALTIME inline void forward(const in_type& ins) noexcept
    {
        for(int i = 0; i < in_sizet; ++i)
        {
            extendedInHt1Vec(i) = ins(i);
        }

        /**
         * | f  |   | Wf  Uf  Bf  |   | input |
         * | i  | = | Wi  Ui  Bi  | * | ht1   |
         * | o  |   | Wo  Uo  Bo  |   | 1     |
         * | ct |   | Wct Uct Bct |
         */
        fioctsVecs.noalias() = combinedWeights * extendedInHt1Vec;

        fioVecs = MathsProvider::sigmoid(fioctsVecs.segment(0, 3 * out_sizet));
        ctVec = MathsProvider::tanh(fioctsVecs.segment(3 * out_sizet, out_sizet));

        computeOutputs();
    }

    /**
     * Sets the layer kernel weights.
     *
     * The weights vector must have size weights[in_size][4 * out_size]
     */
    RTNEURAL_REALTIME void setWVals(const std::vector<std::vector<T>>& wVals);

    /**
     * Sets the layer recurrent weights.
     *
     * The weights vector must have size weights[out_size][4 * out_size]
     */
    RTNEURAL_REALTIME void setUVals(const std::vector<std::vector<T>>& uVals);

    /**
     * Sets the layer bias.
     *
     * The bias vector must have size weights[4 * out_size]
     */
    RTNEURAL_REALTIME void setBVals(const std::vector<T>& bVals);


    //VIC
    /**
     * Sets the value of the output directly.
     *
     * This method directly assigns the provided values to the `outs` member variable.
     * Use this option if you want to set the output to a specific value after the
     * forward pass or from another part of your code.
     *
     * @param new_outs The new values to assign to `outs`.
     */
    RTNEURAL_REALTIME void setInitialHiddenState(const Eigen::Matrix<T, out_size, 1>& new_outs) {
        outs.noalias() = new_outs;
    }

    //VIC
    /**
     * Sets the value of the ctVec directly.
     *
     * This method directly assigns the provided values to the `ctVec` member variable.
     * Use this option if you want to set the output to a specific value after the
     * forward pass or from another part of your code.
     *
     * @param new_outs The new values to assign to `ctVec`.
     */
    RTNEURAL_REALTIME void setInitialCellState(const Eigen::Matrix<T, out_size, 1>& new_ct) {
        ctVec.noalias() = new_ct;
    }

    Eigen::Map<out_type, RTNeuralEigenAlignment> outs;

private:
    T outs_internal alignas(RTNEURAL_DEFAULT_ALIGNMENT)[out_size];

    template <SampleRateCorrectionMode srCorr = sampleRateCorr>
    inline std::enable_if_t<srCorr == SampleRateCorrectionMode::None, void>
    computeOutputs() noexcept
    {
        computeOutputsInternal(cVec, outs);

        for(int i = 0; i < out_sizet; ++i)
        {
            extendedInHt1Vec(in_sizet + i) = outs(i);
        }
    }

    template <SampleRateCorrectionMode srCorr = sampleRateCorr>
    inline std::enable_if_t<srCorr != SampleRateCorrectionMode::None, void>
    computeOutputs() noexcept
    {
        computeOutputsInternal(ct_delayed[delayWriteIdx], outs_delayed[delayWriteIdx]);

        processDelay(ct_delayed, cVec, delayWriteIdx);
        processDelay(outs_delayed, outs, delayWriteIdx);

        for(int i = 0; i < out_sizet; ++i)
        {
            extendedInHt1Vec(in_sizet + i) = outs(i);
        }
    }

    template <typename VecType1, typename VecType2>
    inline void computeOutputsInternal(VecType1& cVecLocal, VecType2& outsVec) noexcept
    {
        cVecLocal.noalias()
            = fioVecs.segment(0, out_sizet)
                  .cwiseProduct(cVec)
            + fioVecs.segment(out_sizet, out_sizet)
                  .cwiseProduct(ctVec);

        cTanhVec = MathsProvider::tanh(cVecLocal);
        outsVec.noalias() = fioVecs.segment(out_sizet * 2, out_sizet).cwiseProduct(cTanhVec);
    }

    template <typename OutVec, SampleRateCorrectionMode srCorr = sampleRateCorr>
    inline std::enable_if_t<srCorr == SampleRateCorrectionMode::NoInterp, void>
    processDelay(std::vector<out_type>& delayVec, OutVec& out, int delayWriteIndex) noexcept
    {
        out = delayVec[0];

        for(int j = 0; j < delayWriteIndex; ++j)
            delayVec[j] = delayVec[j + 1];
    }

    template <typename OutVec, SampleRateCorrectionMode srCorr = sampleRateCorr>
    inline std::enable_if_t<srCorr == SampleRateCorrectionMode::LinInterp, void>
    processDelay(std::vector<out_type>& delayVec, OutVec& out, int delayWriteIndex) noexcept
    {
        out = delayPlus1Mult * delayVec[0] + delayMult * delayVec[1];

        for(int j = 0; j < delayWriteIndex; ++j)
            delayVec[j] = delayVec[j + 1];
    }

    // kernel weights
    weights_combined_type combinedWeights;
    extended_in_out_type extendedInHt1Vec;
    four_out_type fioctsVecs;
    three_out_type fioVecs;
    out_type cTanhVec;

    // intermediate values
    out_type ctVec;
    out_type cVec;

    // needed for delays when doing sample rate correction
    std::vector<out_type> ct_delayed;
    std::vector<out_type> outs_delayed;
    int delayWriteIdx = 0;
    T delayMult = (T)1;
    T delayPlus1Mult = (T)0;
};




template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
CustomLSTMLayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::CustomLSTMLayerT()
    : outs(outs_internal)
{
    combinedWeights = weights_combined_type::Zero();
    extendedInHt1Vec = extended_in_out_type::Zero();
    fioctsVecs = four_out_type::Zero();
    fioVecs = three_out_type::Zero();

    ctVec = out_type::Zero();
    cTanhVec = out_type::Zero();

    reset();
}

template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
template <SampleRateCorrectionMode srCorr>
std::enable_if_t<srCorr == SampleRateCorrectionMode::NoInterp, void>
CustomLSTMLayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::prepare(int delaySamples)
{
    delayWriteIdx = delaySamples - 1;
    ct_delayed.resize(delayWriteIdx + 1, {});
    outs_delayed.resize(delayWriteIdx + 1, {});

    reset();
}

template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
template <SampleRateCorrectionMode srCorr>
std::enable_if_t<srCorr == SampleRateCorrectionMode::LinInterp, void>
CustomLSTMLayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::prepare(T delaySamples)
{
    const auto delayOffFactor = delaySamples - std::floor(delaySamples);
    delayMult = (T)1 - delayOffFactor;
    delayPlus1Mult = delayOffFactor;

    delayWriteIdx = (int)std::ceil(delaySamples) - (int)std::ceil(delayOffFactor);
    ct_delayed.resize(delayWriteIdx + 1, {});
    outs_delayed.resize(delayWriteIdx + 1, {});

    reset();
}

template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
void CustomLSTMLayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::reset()
{
    if(sampleRateCorr != SampleRateCorrectionMode::None)
    {
        for(auto& x : ct_delayed)
            x = out_type::Zero();

        for(auto& x : outs_delayed)
            x = out_type::Zero();
    }

    // reset output state
    extendedInHt1Vec.setZero();
    extendedInHt1Vec(in_sizet + out_sizet) = (T)1;
    outs = out_type::Zero();
    cVec = out_type::Zero();
    ctVec.setZero();
}

// kernel weights
template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
void CustomLSTMLayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::setWVals(const std::vector<std::vector<T>>& wVals)
{
    for(int i = 0; i < in_size; ++i)
    {
        for(int k = 0; k < out_size; ++k)
        {
            combinedWeights(k, i) = wVals[i][k + out_sizet]; // Wf
            combinedWeights(k + out_sizet, i) = wVals[i][k]; // Wi
            combinedWeights(k + out_sizet * 2, i) = wVals[i][k + out_sizet * 3]; // Wo
            combinedWeights(k + out_sizet * 3, i) = wVals[i][k + out_sizet * 2]; // Wc
        }
    }
}

// recurrent weights
template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
void CustomLSTMLayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::setUVals(const std::vector<std::vector<T>>& uVals)
{
    int col;
    for(int i = 0; i < out_size; ++i)
    {
        col = i + in_sizet;
        for(int k = 0; k < out_size; ++k)
        {
            combinedWeights(k, col) = uVals[i][k + out_sizet]; // Uf
            combinedWeights(k + out_sizet, col) = uVals[i][k]; // Ui
            combinedWeights(k + out_sizet * 2, col) = uVals[i][k + out_sizet * 3]; // Uo
            combinedWeights(k + out_sizet * 3, col) = uVals[i][k + out_sizet * 2]; // Uc
        }
    }
}

// biases
template <typename T, int in_sizet, int out_sizet, SampleRateCorrectionMode sampleRateCorr, typename MathsProvider>
void CustomLSTMLayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::setBVals(const std::vector<T>& bVals)
{
    int col = in_size + out_size;
    for(int k = 0; k < out_size; ++k)
    {
        combinedWeights(k, col) = bVals[k + out_sizet]; // Bf
        combinedWeights(k + out_sizet, col) = bVals[k]; // Bi
        combinedWeights(k + out_sizet * 2, col) = bVals[k + out_sizet * 3]; // Bo
        combinedWeights(k + out_sizet * 3, col) = bVals[k + out_sizet * 2]; // Bc
    }
}

} // namespace RTNEURAL_NAMESPACE
