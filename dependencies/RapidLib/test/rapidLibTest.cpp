#include <vector>
#include <vector>
#include <iostream>
#include <cassert>
#include <random>
#include <algorithm>
//#include <filesystem>

#include "../src/rapidLib.h"

int main(int argc, const char * argv[]) 
{
    ///////////////////////////////////////////////////////////////////////////////////////////
    // Bayes test

    rapidLib::rapidStream<double> rapidProcess;
    rapidProcess.bayesSetDiffusion(-2.0);
    rapidProcess.bayesSetJumpRate(-10.0);
    rapidProcess.bayesSetMVC(1.);
    
    double bayes = 0.;

    for (int i = 0; i < 100; ++i) 
    {
        bayes = rapidProcess.bayesFilter(i/100.);
        //std::cout << "bayes: " << bayes <<std::endl;
    }
    assert( bayes > 0.68 );
    std::cout << "----- Bayes test passed." << std::endl; // Bayes test

    ///////////////////////////////////////////////////////////////////////////////////////////
    //test empty set
    rapidLib::seriesClassification emptyDTW;
    rapidLib::trainingSeries tempSeriesTestEmpty;

    for (size_t i = 0; i < 5; ++i) 
    {
        tempSeriesTestEmpty.input.push_back({ 0.1, 0.1, 0.1 });
    }

    assert(emptyDTW.run(tempSeriesTestEmpty.input) == "none");
    std::cout << "----- DTW empty test passed." << std::endl;

    ///////////////////////////////////////////////////////////////////////////////////////////
    //vanAllenTesting
    rapidLib::seriesClassification testDTW;
    std::vector<rapidLib::trainingSeries> testVector;
    rapidLib::trainingSeries tempSeriesTest;

    for (std::size_t i = 0; i < 5; ++i) 
    {
        tempSeriesTest.input.push_back({ 0.1, 0.1, 0.1 });
    }
    tempSeriesTest.label = "zzz";
    testVector.push_back(tempSeriesTest);
    tempSeriesTest.label = "yyy";
    testVector.push_back(tempSeriesTest);

    testDTW.train(testVector);
    assert(testDTW.run(tempSeriesTest.input) == "zzz");
    std::cout << "----- DTW test passed." << std::endl;

    ///////////////////////////////////////////////////////////////////////////////////////////
    //test for Louis

    //#define MULTILAYER 1
#ifdef MULTILAYER

    rapidLib::regression hiddenNN;

    std::vector<rapidLib::trainingExample> trainingSetHN;
    rapidLib::trainingExample  tempExampleHN;

    for (std::size_t i = 0; i < 1000; ++i)
    {
        tempExampleHN.input = { double(i) };
        tempExampleHN.output = { double(i) };
        trainingSetHN.push_back(tempExampleHN);
    }

    hiddenNN.setNumHiddenLayers(2);
    hiddenNN.setNumHiddenNodes(2);
    hiddenNN.setNumEpochs(1000);

    hiddenNN.train(trainingSetHN);
    hiddenNN.train(trainingSetHN);

    std::cout << "----- Louis test passed." << std::endl;

    //////////////////////////////////////////////////////////////////////////////simple multilayer test

    rapidLib::regression myNN_ML1;
    rapidLib::regression myNN_ML2;

    std::vector<rapidLib::trainingExample> trainingSet1;
    rapidLib::trainingExample  tempExample1;
    tempExample1.input = { 1.0, 1.0, 1.0 };
    tempExample1.output = { 10.0 };
    trainingSet1.push_back(tempExample1);
    tempExample1.input = { 2.0, 2.0, 2.0 };
    tempExample1.output = { 1.3 };
    trainingSet1.push_back(tempExample1);

    assert(myNN_ML1.getTrainingProgress()[0] = 0);
    assert(myNN_ML2.getTrainingProgress()[0] = 0);

    myNN_ML2.setNumHiddenLayers(2);
    assert(myNN_ML2.getNumHiddenLayers()[0] == 2);
    myNN_ML2.setNumEpochs(1000);
    assert(myNN_ML2.getNumEpochs()[0] == 1000);

    myNN_ML1.train(trainingSet1);
    myNN_ML2.train(trainingSet1);

    assert(myNN_ML1.getTrainingProgress()[0] = 1); //TODO: Should test this in progress
    assert(myNN_ML2.getTrainingProgress()[0] = 1);

    std::vector<double> inputVec1 = { 1.1, 1.1, 1.1 };
    std::cout << "single layer: " << myNN_ML1.run(inputVec1)[0] << std::endl;
    std::cout << "multilayer: " << myNN_ML2.run(inputVec1)[0] << std::endl;
    //assert(myNN_ML1.run(inputVec1)[0] == myNN_ML2.run(inputVec1)[0]);

    /*
    myNN2.reset();
    trainingSet1.clear();
    tempExample1.input = {0., 0. };
    tempExample1.output = { 0.0 };
    trainingSet1.push_back(tempExample1);
    tempExample1.input = {0., 1. };
    tempExample1.output = { 1.0 };
    trainingSet1.push_back(tempExample1);
    tempExample1.input = {1., 0. };
    tempExample1.output = { 1.0 };
    trainingSet1.push_back(tempExample1);
    tempExample1.input = {1., 1. };
    tempExample1.output = { 2.0 };
    trainingSet1.push_back(tempExample1);
    myNN2.setNumHiddenLayers(2);
    assert(myNN2.getNumHiddenLayers()[0] == 2);

    myNN2.setNumEpochs(500000);
    myNN2.train(trainingSet1);

    inputVec1 = { 0.9, 0.7 };
    std::cout << myNN2.run(inputVec1)[0] <<std::endl;
     */
#else
    std::cout << "----- Multilayer tests skipped." << std::endl;
#endif
     ////////////////////////////////////////////////////////////////////////////////

    rapidLib::regression myNN;
    rapidLib::regression myNN_nodes;
    myNN_nodes.setNumHiddenNodes(10);
    assert(myNN_nodes.getNumHiddenNodes()[0] == 10);
    rapidLib::classification myKnn;
    //classification mySVM(classification::svm);
    
    std::vector<rapidLib::trainingExample> trainingSet;
    rapidLib::trainingExample  tempExample;
    tempExample.input = { 0.2, 0.7 };
    tempExample.output = { 3.0 };
    trainingSet.push_back(tempExample);

    tempExample.input = { 2.0, 44.2 };
    tempExample.output = { 20.14 };
    trainingSet.push_back(tempExample);

    myNN.train(trainingSet);
    myNN_nodes.train(trainingSet);
    //std::cout << myNN.getJSON() << std::endl;
    //std::string filepath = std::filesystem::temp_directory_path().string() + "/NN_test.json";
    std::string filepath = "NN_test.json";
    myNN.writeJSON(filepath);
 
    rapidLib::regression myNNfromString;
    myNNfromString.putJSON(myNN.getJSON());

    rapidLib::regression myNNfromFile;;
    myNNfromFile.readJSON(filepath);
    std::vector<double> inputVec = { 2.0, 44.2 };


    std::cout << "before: " << myNN.run(inputVec)[0] << std::endl;
    //std::cout << "from string: " << myNNfromString.run(inputVec)[0] << std::endl;
    //std::cout << "from file: " << myNNfromFile.run(inputVec)[0] << std::endl;

    assert(myNN.run(inputVec)[0] == 20.14);
    assert(myNN_nodes.run(inputVec)[0] == 20.14);
    assert(myNN.run(inputVec)[0] == myNNfromString.run(inputVec)[0]);
    assert(myNN.run(inputVec)[0] == myNNfromFile.run(inputVec)[0]);

    //Training Bug 2020?
    myNN_nodes.train(trainingSet);
    assert(myNN_nodes.run(inputVec)[0] == 20.14);
    
    //Testing exceptions for regression
    std::vector<double> emptyVec = {};
    try 
    {
        myNN.run(emptyVec);
    }
    catch (const std::length_error& e) 
    {
        assert(e.what() == std::string("bad input size: 0"));
    }

    std::vector<double> wrongSizeVector = { 1, 1, 1, 1, 1, 1 };
    try 
    {
        myNN.run(wrongSizeVector);
    }
    catch (const std::length_error& e) 
    {
        assert(e.what() == std::string("bad input size: 6"));
    }
    std::cout << "----- Regression run exceptions passed." << std::endl;

    rapidLib::regression badNN;
    std::vector<rapidLib::trainingExample> badSet;
    rapidLib::trainingExample  badExample;
    badExample.input = { 0.1, 0.2 };
    badExample.output = { 3.0 };
    badSet.push_back(badExample);

    badExample.input = { 1.0, 2.0, 3.0 };
    badExample.output = { 4.0 };
    badSet.push_back(badExample);

    try 
    {
        badNN.train(badSet);
    }
    catch (const std::length_error& e) 
    {
        assert(e.what() == std::string("unequal feature vectors in input."));
    }

    badSet.clear();
    badExample.input = { 0.1, 0.2 };
    badExample.output = { 3.0 };
    badSet.push_back(badExample);

    badExample.input = { 1.0, 2.0 };
    badExample.output = { 4.0, 5.0 };
    badSet.push_back(badExample);

    try 
    {
        badNN.train(badSet);
    }
    catch (const std::length_error& e) 
    {
        assert(e.what() == std::string("unequal output vectors."));
    }
    std::cout << "----- Regression train exceptions passed." << std::endl;

    ///////////////////////////

    myKnn.train(trainingSet);
    //mySVM.train(trainingSet);

    //   std::cout << myKnn.getJSON() << std::endl;
   // std::string filepath2 = std::filesystem::temp_directory_path().string() + "/modelSetDescription_knn.json";
    std::string filepath2 = "modelSetDescription_knn.json";
    myKnn.writeJSON(filepath2);

    rapidLib::classification myKnnFromString(rapidLib::classification::knn);
    myKnnFromString.putJSON(myKnn.getJSON());

    rapidLib::classification myKnnFromFile;
    myKnnFromFile.readJSON(filepath2);

    std::cout << "knn before: " << myKnn.run(inputVec)[0] << std::endl;
    //std::cout << "svm: " << mySVM.run(inputVec)[0] << std::endl;
    //std::cout << "knn from string: " << myKnnFromString.run(inputVec)[0] << std::endl;
    //std::cout << "knn from file: " << myKnnFromFile.run(inputVec)[0] << std::endl;

    assert(myKnn.run(inputVec)[0] == 20);
    assert(myKnn.run(inputVec)[0] == myKnnFromString.run(inputVec)[0]);
    assert(myKnn.run(inputVec)[0] == myKnnFromFile.run(inputVec)[0]);

    std::cout << "These errors are part of the test:" << std::endl;
    try 
    {
        myKnn.run(emptyVec);
    }
    catch (const std::length_error& e) 
    {
        std::cout << "error: " << e.what() << std::endl;
        assert(e.what() == std::string("bad input size: 0"));
    }
    try 
    {
        myKnn.run(wrongSizeVector);
    }
    catch (const std::length_error& e) 
    {
        std::cout << "error: " << e.what() << std::endl;
        assert(e.what() == std::string("bad input size: 6"));
    }
    std::cout << "----- KNN run exceptions passed." << std::endl;

    assert(myKnn.getK()[0] == 1);
    myKnn.setK(0, 2);
    assert(myKnn.getK()[0] == 2);
    std::cout << "----- KNN get/set K passed." << std::endl;

    //    regression<float> bigVector;
    std::vector<rapidLib::trainingExampleFloat > trainingSet2;
    rapidLib::trainingExampleFloat tempExample2;
    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution(-0.5, 0.5);
    int vecLength = 64;
    for (std::size_t j = 0; j < vecLength; ++j) 
    {
        tempExample2.input.clear();
        tempExample2.output.clear();
        for (std::size_t i = 0; i < vecLength; ++i) 
        {
            tempExample2.input.push_back(distribution(generator));
        }
        tempExample2.output = { distribution(generator) };
        trainingSet2.push_back(tempExample2);
    }
    //    bigVector.train(trainingSet2);
    std::vector<float> inputVec2;
    for (std::size_t i = 0; i < vecLength; ++i) 
    {
        inputVec2.push_back(distribution(generator));
    }
    //    assert (isfinite(bigVector.run(inputVec2)[0]));
    std::cout << "----- What is this test?" << std::endl;

    /////////

    /*
     classification mySVM2(classification::svm);

     std::vector<rapidLib::trainingExample> trainingSet3;
     rapidLib::trainingExample tempExample3;

     tempExample3.input = { 0., 0. };
     tempExample3.output = { 0. };
     trainingSet3.push_back(tempExample3);

     tempExample3.input = { 1., 0. };
     tempExample3.output = { 1. };
     trainingSet3.push_back(tempExample3);

     tempExample3.input = { 1., 8. };
     tempExample3.output = { 5. };
     trainingSet3.push_back(tempExample3);
     */


     /*
      tempExample3.input = { 1., 1. };
      tempExample3.output = { 2. };
      trainingSet3.push_back(tempExample3);
      */

      //mySVM2.train(trainingSet3);

    std::vector<double>inputVec4 = { 1., 0. };
    //std::cout << "svm: " << mySVM2.run(inputVec4)[0] << std::endl;

    std::vector<double> inputVec3 = { 0., 0. };
    //std::cout << "svm2: " << mySVM2.run(inputVec3)[0] << std::endl;



    //////////////////////////////////////////////////////////////////////// DTW

    //Test series
    std::vector<std::vector<double>> seriesOne;
    seriesOne.push_back({ 1., 5. });
    seriesOne.push_back({ 2., 4. });
    seriesOne.push_back({ 3., 3. });
    seriesOne.push_back({ 4., 2. });
    seriesOne.push_back({ 5., 1. });

    std::vector<std::vector<double>> seriesTwo;
    seriesTwo.push_back({ 1., 4. });
    seriesTwo.push_back({ 2., -3. });
    seriesTwo.push_back({ 1., 5. });
    seriesTwo.push_back({ -2., 1. });


    //Testing with labels
    rapidLib::seriesClassification myDTW;
    std::vector<rapidLib::trainingSeries> seriesVector;
    rapidLib::trainingSeries tempSeries;

    tempSeries.input.push_back({ 1., 5. });
    tempSeries.input.push_back({ 2., 4. });
    tempSeries.input.push_back({ 3., 3. });
    tempSeries.input.push_back({ 4., 2. });
    tempSeries.input.push_back({ 5., 1. });
    tempSeries.label = "first series";
    seriesVector.push_back(tempSeries);

    tempSeries = {};
    tempSeries.input.push_back({ 1., 4. });
    tempSeries.input.push_back({ 2., -3. });
    tempSeries.input.push_back({ 1., 5. });
    tempSeries.input.push_back({ -2., 1. });
    tempSeries.label = "second series";
    seriesVector.push_back(tempSeries);

    myDTW.train(seriesVector);
    assert(myDTW.run(seriesOne) == "first series");
    assert(myDTW.run(seriesTwo) == "second series");
    std::cout << "----- Single thread DTW tests passed." << std::endl;
    std::cout << myDTW.getCosts()[0] << std::endl;
    std::cout << myDTW.getCosts()[1] << std::endl;

    assert(myDTW.runParallel(seriesOne) == "first series");
    assert(myDTW.runParallel(seriesTwo) == "second series");
    std::cout << "----- Parallel DTW tests passed." << std::endl;
    std::cout << myDTW.getCosts()[0] << std::endl;
    std::cout << myDTW.getCosts()[1] << std::endl;
    

    //testing match against single label
    assert(myDTW.run(seriesOne, "second series") == 19.325403217417502);
    std::cout << "----- DTW single label passed." << std::endl; //FIXME
    //assert(myDTW.runParallel(seriesOne, "second series") == 19.325403217417502);
    std::cout << "----- DTW parallel single label CRASHES!!!!." << std::endl; //FIXME

    //Training set stats
    assert(myDTW.getMaxLength() == 5);
    assert(myDTW.getMinLength() == 4);
    assert(myDTW.getMaxLength("first series") == 5);
    assert(myDTW.getMinLength("first series") == 5);
    assert(myDTW.getMaxLength("second series") == 4);
    assert(myDTW.getMinLength("second series") == 4);
    std::cout << "----- DTW stats pass ." << std::endl;

    //costs inside of a series
    tempSeries = {};
    tempSeries.input.push_back({ 1., 5.1 });
    tempSeries.input.push_back({ 2., 4.1 });
    tempSeries.input.push_back({ 3., 3.1 });
    tempSeries.input.push_back({ 4., 2.1 });
    tempSeries.input.push_back({ 5., 1.1 });
    tempSeries.label = "first series";
    seriesVector.push_back(tempSeries);

    tempSeries = {};
    tempSeries.input.push_back({ 1.3, 5.1 });
    tempSeries.input.push_back({ 2.3, 4.1 });
    tempSeries.input.push_back({ 3.3, 3.1 });
    tempSeries.input.push_back({ 4.3, 2.1 });
    tempSeries.input.push_back({ 5.3, 1.1 });
    tempSeries.label = "first series";
    seriesVector.push_back(tempSeries);

    myDTW.train(seriesVector);
    std::cout << "mincost " << myDTW.calculateCosts("first series").min << std::endl;
    std::cout << "maxcost " << myDTW.calculateCosts("first series").max << std::endl;

    /*
     fastDTW fastDtw;
     std::cout << "fast one-two cost " << fastDtw.getCost(seriesOne, seriesTwo, 1) << std::endl;
     std::cout << "fast two-one cost " << fastDtw.getCost(seriesTwo, seriesOne, 1) << std::endl;

     dtw slowDTW;
     std::cout << "slow one-two cost " << slowDTW.getCost(seriesOne, seriesTwo) << std::endl;
     */

     //Long dtw test
    std::size_t testSize = 800;
    seriesVector.clear();
    std::vector<std::vector<double> > inputSeries;
    tempSeries.input.clear();
    for (std::size_t i = 0; i < testSize; ++i) 
    {
        tempSeries.input.push_back({ (double)i, (double)i });
    }
    tempSeries.label = "long up";
    seriesVector.push_back(tempSeries);

    tempSeries.input.clear();
    for (std::size_t i = 0; i < testSize; ++i) 
    {
        tempSeries.input.push_back({ (double)i, double(testSize - i) });
    }
    tempSeries.label = "long down";
    seriesVector.push_back(tempSeries);

    myDTW.train(seriesVector);
    inputSeries = tempSeries.input;
    assert(myDTW.run(inputSeries) == "long down");
    //std::cout << myDTW.getCosts()[0] << std::endl;
    //std::cout << myDTW.getCosts()[1] << std::endl;

  ////////////////////////////////////////////////////////////////////////
  
  rapidLib::classification setTest;
  try
  {
    setTest.setK(0, 2);
  }
  catch (...)
  {
    std::cout << "setting k on empty set doesn't crash" <<std::endl;
  }
    
    ////////////////////////////////////////////////////////////////////////
//#define layerTest 1
#ifdef layerTest
    //Machine Learning
    rapidLib::regression mtofRegression; //Create a machine learning object
    mtofRegression.setNumHiddenLayers(2);
    std::cout << "epochs: " << mtofRegression.getNumEpochs()[0] << std::endl;
    mtofRegression.setNumEpochs(5000);

    std::vector<rapidLib::trainingExample> trainingSet_mtof;
    rapidLib::trainingExample  tempExample_mtof;

    //Setting up the first element of training data
    tempExample_mtof.input = { 48 };
    tempExample_mtof.output = { 130.81 };
    trainingSet_mtof.push_back(tempExample_mtof);

    //More elements
    tempExample_mtof.input = { 54 };
    tempExample_mtof.output = { 185.00 };
    trainingSet_mtof.push_back(tempExample_mtof);

    tempExample_mtof.input = { 60 };
    tempExample_mtof.output = { 261.63 };
    trainingSet_mtof.push_back(tempExample_mtof);

    tempExample_mtof.input = { 66 };
    tempExample_mtof.output = { 369.994 };
    trainingSet_mtof.push_back(tempExample_mtof);

    tempExample_mtof.input = { 72 };
    tempExample_mtof.output = { 523.25 };
    trainingSet_mtof.push_back(tempExample_mtof);

    //Train the machine learning model with the data
    mtofRegression.train(trainingSet_mtof);

    //Get some user input
    char newNote { 0 };
    std::cout << "Type a MIDI note number.\n"; std::cin >> newNote;

    //Run the trained model on the user input
    std::vector<double> inputVec_mtof = { double(newNote) };
    double freqHz { mtofRegression.run(inputVec_mtof)[0] };

    std::cout << "MIDI note " << newNote << " is " << freqHz << " Hertz" << std::endl;
#endif

    ///////////////////////////////////////////////////////////////////////////////////////////////
    
    return 0;
}
