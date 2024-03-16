#include "RTNeuralGuitarLSTM.h"

using Vec2d = std::vector<std::vector<float>>;

Vec2d transpose(const Vec2d& x)
{
    auto outer_size = x.size();
    auto inner_size = x[0].size();
    Vec2d y(inner_size, std::vector<float>(outer_size, 0.0f));

    for (size_t i = 0; i < outer_size; ++i)
    {
        for (size_t j = 0; j < inner_size; ++j)
            y[j][i] = x[i][j];
    }

    return y;
}

template <typename T1>
void RT_GuitarLSTM::set_weights(T1 model, const char* filename)
{
    // Initialize the model
    auto& lstm = (*model).template get<2>();
    auto& dense = (*model).template get<3>();

    // read a JSON file
    std::ifstream i2(filename);
    nlohmann::json weights_json;
    i2 >> weights_json;

    Vec2d lstm_weights_ih = weights_json["/state_dict/rec.weight_ih_l0"_json_pointer];
    lstm.setWVals(transpose(lstm_weights_ih));

    Vec2d lstm_weights_hh = weights_json["/state_dict/rec.weight_hh_l0"_json_pointer];
    lstm.setUVals(transpose(lstm_weights_hh));

    std::vector<float> lstm_bias_ih = weights_json["/state_dict/rec.bias_ih_l0"_json_pointer];
    std::vector<float> lstm_bias_hh = weights_json["/state_dict/rec.bias_hh_l0"_json_pointer];
    for (int i = 0; i < 80; ++i)
        lstm_bias_hh[i] += lstm_bias_ih[i];
    lstm.setBVals(lstm_bias_hh);

    Vec2d dense_weights = weights_json["/state_dict/lin.weight"_json_pointer];
    dense.setWeights(dense_weights);

    std::vector<float> dense_bias = weights_json["/state_dict/lin.bias"_json_pointer];
    dense.setBias(dense_bias.data());

}
void RT_GuitarLSTM::load_json(const char* filename)
{
    // Read in the JSON file
    std::ifstream i2(filename);
	nlohmann::json weights_json;
	i2 >> weights_json;

    // Get the input size of the JSON file
	// int input_size_json = weights_json["/model_data/input_size"_json_pointer];
	// input_size = input_size_json;

    set_weights(&model, filename);
}

int RT_GuitarLSTM::getInputSize()
{
    return input_size;
}


void RT_GuitarLSTM::reset()
{
    model.reset();
}
