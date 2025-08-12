"use strict";
let expect = require('chai').expect;
let rapidMix = require('../rapidLib/RapidLib.js');
//import RapidLib from '../rapidLib/RapidLib.js';

//let jsons = require('./modelSetDescription.json');

///////////////////////////////////////////// vanAllen bug
let testDTW = new rapidMix.SeriesClassification();

let testSetXXX = [];
for (let i = 0; i < 111; ++i) {
    testSetXXX.push([0.1, 0.1, 0.1]);
}

let series2 = {input: testSetXXX, label: "yyy"};
let series1 = {input: testSetXXX, label: "zzz"};
let sset = [series1, series2];

testDTW.train(sset);
testDTW.run(testSetXXX);

///////////////////////////////////////////////


let testSet = [
    {
        input: [0, 0],
        output: [0]
    },
    {
        input: [0, 1],
        output: [1]
    },
    {
        input: [1, 0],
        output: [1]
    },
    {
        input: [1, 1],
        output: [2]
    },
];

let testSet2 = [
    {
        input: [0, 0],
        output: [0, 9]
    },
    {
        input: [0, 1],
        output: [1, 2]
    },
    {
        input: [1, 0],
        output: [1, 2]
    },
    {
        input: [1, 1],
        output: [2, 4]
    },
];

let testSet3 = [
    {
        input: [8],
        output: [5]
    },
    {
        input: [2],
        output: [3]
    },
];

let testSet4 = [
    {
        input: [1.0, 1.0, 1.0],
        output: [10.]
    },
    {
        input: [2.0, 2.0, 2.0],
        output: [1.3]
    },
];


let badSet = [
    {
        input: [1, 2, 3, 4],
        output: [5]
    },
    {
        input: [6],
        output: [7]
    }
];

let testSeries = {
    input: [[1., 5.],
        [2., 4.],
        [3., 3.],
        [4., 2.],
        [1., 5.]],
    label: "testSeries"
};

let testSeries2 = {
    input: [[1., 4.],
        [2., -3.],
        [1., 5.],
        [-2., 1.]],
    label: "testSeries2"
};

let inputSeries = [[1., 4.], [2., -3.], [1., 5.], [-2., 1.]];

describe('RapidLib Machine Learning', function () {
    describe('Regression', function () {
        let myRegression = new rapidMix.Regression();
        it('should create a new Regression object', function () {
            expect(myRegression).to.be.an.instanceof(rapidMix.Regression);
        });
        it('should create a new object with a modelSet', function () {
            expect(myRegression).to.have.property('modelSet');
        });
        it('should reject malformed training data', function () {
            let trained = myRegression.train(badSet);
            expect(trained).to.be.false;
        });
            let trained = myRegression.train(testSet);
            expect(trained).to.be.true;
        });
        it('run() should return expected results', function () {
            let response1 = myRegression.run([0, 0]);
            expect(response1[0]).to.be.below(0.000001); //close enough
            let response2 = myRegression.run([0.2789, 0.4574]);
            expect(response2[0]).to.equal(0.6737399669524929);
            let response3 = myRegression.run(0.9, 0.7); //likes a list as well as an array
            expect(response3[0]).to.equal(1.6932444207337964);
        });
        it('should get and set hidden nodes, and still return good results', function () {
            let myRegNodes = new rapidMix.Regression();
            myRegNodes.setNumHiddenNodes(12);
            expect(myRegNodes.getNumHiddenNodes()).to.equal(12);
            myRegNodes.train(testSet);
            let response2 = myRegression.run([0.2789, 0.4574]);
            expect(response2[0]).to.equal(0.6737399669524929);
        });
        it('should return zero on input that doesn\'t match numInputs', function () {
            let response1 = myRegression.run([33, 2, 44, 9]);
            expect(response1[0]).to.equal(0);
            let response2 = myRegression.run([1]);
            expect(response2[0]).to.equal(0);
        });
        it('should work with multiple outputs', function () {
            let myReg2 = new rapidMix.Regression();
            expect(myReg2).to.be.an.instanceof(rapidMix.Regression);
            expect(myReg2).to.have.property('modelSet');
            let trained = myReg2.train(testSet2);
            expect(trained).to.be.true;
            let response1 = myReg2.run([0, 0]);
            expect(response1[0]).to.be.below(0.000001); //close enough
            expect(response1[1]).to.be.above(8.99); //close enough
            let response2 = myReg2.run([0.2789, 0.4574]);
            expect(response2[0]).to.equal(0.6737399669524929);
            expect(response2[1]).to.equal(2.2184955630745637);
            let response3 = myReg2.run(0.9, 0.7); //NB: this is not an array
            expect(response3[0]).to.equal(1.6932444207337964);
            expect(response3[1]).to.equal(3.4050876960817114);
        });
        it('can be initialized', function () {
            myRegression.reset();
            let response2 = myRegression.run([0.2789, 0.4574]);
            expect(response2[0]).to.equal(0); //initialized models return 0
        });
        it('clears properly when retraining', function () {
            let myReg = new rapidMix.Regression();
            myReg.train(testSet);
            let response = myReg.run([1, 1]);
            expect(response[0]).to.equal(2.0000000127072584);
            myReg.train(testSet3);
            let response2 = myReg.run([8]);
            expect(response2[0]).to.equal(5);

        });
        it('runs with multiple layers', function () {
            let myMLP = new rapidMix.Regression();
            let numLayers = 2;
            myMLP.setNumHiddenLayers(numLayers);
            expect(myMLP.getNumHiddenLayers()).to.equal(numLayers);
            myMLP.setNumEpochs(5000);
            let trained = myMLP.train(testSet4);
            expect(trained).to.be.true;
            let response = myMLP.run([2.0, 2.0, 2.0]);
            expect(response[0]).to.equal(1.3000000000000007);

        })
    });

    describe('KNN Classification', function () {
        let myClassification = new rapidMix.Classification();
        it('should create a new Classification object', function () {
            expect(myClassification).to.be.an.instanceof(rapidMix.Classification);
        });
        it('should create a new object with a modelSet', function () {
            expect(myClassification).to.have.property('modelSet');
        });
        it('should reject malformed training data', function () {
            let trained = myClassification.train(badSet);
            expect(trained).to.be.false;
        });
        it('should train the modelSet with good data', function () {
            let trained = myClassification.train(testSet);
            expect(trained).to.be.true;
        });
        it('run() should return expected results', function () {
            let response1 = myClassification.run([0, 0]);
            expect(response1[0]).to.be.equal(0);
            let response2 = myClassification.run([0.8789, 0.1574]);
            expect(response2[0]).to.equal(1);
            let response3 = myClassification.run([0.9, 0.7]);
            expect(response3[0]).to.equal(2);
        });
        it('should return zero on input that doesn\'t match numInputs', function () {
            let response1 = myClassification.run([33, 2, 44, 9]);
            expect(response1[0]).to.equal(0);
            let response2 = myClassification.run([1]);
            expect(response2[0]).to.equal(0);
        });
        it('should work with multiple outputs', function () {
            let myClass2 = new rapidMix.Classification();
            expect(myClass2).to.be.an.instanceof(rapidMix.Classification);
            expect(myClass2).to.have.property('modelSet');
            let trained = myClass2.train(testSet2);
            expect(trained).to.be.true;
            let response1 = myClass2.run([0, 0]);
            expect(response1[0]).to.equal(0);
            expect(response1[1]).to.equal(9);
            let response2 = myClass2.run([0.8789, 0.1574]);
            expect(response2[0]).to.equal(1);
            expect(response2[1]).to.equal(2);
            let response3 = myClass2.run([0.9, 0.7]);
            expect(response3[0]).to.equal(2);
            expect(response3[1]).to.equal(4);
        });
        it('can be initialized', function () {
            myClassification.reset();
            let response2 = myClassification.run([0.2789, 0.4574]);
            expect(response2[0]).to.equal(0); //initialized models return 0
        });
        it('can get k', function () {
            let myClass = new rapidMix.Classification();
            myClass.train(testSet2);
            let response3 = myClass.getK();
            expect(response3[0]).to.equal(1);
        });
        it('can set k', function () {
            let myClass = new rapidMix.Classification();
            myClass.train(testSet2);
            myClass.setK(0, 3);
            let response3 = myClass.getK();
            expect(response3[0]).to.equal(3);
            myClass.setK(0, 999);
            response3 = myClass.getK();
            expect(response3[0]).to.equal(4);
        });
        it('clears properly when retraining', function () {
            let myClass = new rapidMix.Classification();
            myClass.train(testSet);
            let response = myClass.run([1, 1]);
            expect(response[0]).to.equal(2);
            myClass.train(testSet3);
            let response2 = myClass.run([8]);
            expect(response2[0]).to.equal(5);

        })
    });
    /*
        describe('SVM Classification', function () {
            let mySVM = new rapidMix.Classification(rapidMix.ClassificationTypes.SVM);
            it('should create a new Classification object', function () {
                expect(mySVM).to.be.an.instanceof(rapidMix.Classification);
            });
            it('should create a new object with a modelSet', function () {
                expect(mySVM).to.have.property('modelSet');
            });

            it('should reject malformed training data', function () {
                let trained = mySVM.train(badSet);
                expect(trained).to.be.false;
            });

            it('should train the modelSet with good data', function () {
                let trained = mySVM.train(testSet);
                expect(trained).to.be.true;
            });

            it('run() should return expected results' , function () {
                let response1 = mySVM.run([0, 0]);
                expect(response1[0]).to.be.equal(0);
                let response2 = mySVM.run([0.8789, 0.1574]);
                expect(response2[0]).to.equal(1);
                let response3 = mySVM.run([0.9, 0.7]);
                expect(response3[0]).to.equal(2);
            });
            it('should return zero on input that doesn\'t match numInputs', function () {
                let response1 = mySVM.run([33, 2, 44, 9]);
                expect(response1[0]).to.equal(0);
                let response2 = mySVM.run([1]);
                expect(response2[0]).to.equal(0);
            });

             it('should work with multiple outputs', function () {
             let mySVM2 = new rapidMix.Classification(rapidMix.ClassificationTypes.SVM);
             expect(mySVM2).to.be.an.instanceof(rapidMix.Classification);
             expect(mySVM2).to.have.property('modelSet');
             let trained = mySVM2.train(testSet2);
             expect(trained).to.be.true;
             let response1 = mySVM2.run([0, 0]);
             expect(response1[0]).to.equal(0);
             expect(response1[1]).to.equal(9);
             let response2 = mySVM2.run([0.8789, 0.1574]);
             expect(response2[0]).to.equal(1);
             expect(response2[1]).to.equal(2);
             let response3 = mySVM2.run([0.9, 0.7]);
             expect(response3[0]).to.equal(2);
             expect(response3[1]).to.equal(4);
             });
            it('can be initialized', function () {
                mySVM.reset();
                let response2 = mySVM.run([0.2789, 0.4574]);
                expect(response2[0]).to.equal(0); //initialized models return 0
            });
        });
    */

    describe('ModelSet', function () {
        let myModelSet = new rapidMix.ModelSet();
        it('should create a new ModelSet object', function () {
            expect(myModelSet).to.be.an.instanceof(rapidMix.ModelSet);
        });
        it('should create a new object with a modelSet', function () {
            expect(myModelSet).to.have.property('modelSet');
        });
        // console.log(myModelSet.getJSON());
        it('should load from JSON?');
        it('should process input');
    });

    describe('seriesClassification', function () {
        let myDTW = new rapidMix.SeriesClassification();
        it('should create a new seriesClassification object', function () {
            expect(myDTW).to.be.an.instanceof(rapidMix.SeriesClassification);
        });

        let seriesSet = [testSeries, testSeries2];
        let trained = myDTW.train(seriesSet);
        it('should let me train on a series set', function () {
            expect(trained).to.be.true;
        });

        // it('should correctly identify series 1', function () {
        //      expect(myDTW.run(testSeries.input)).to.equal("testSeries");
        // });

        it('should correctly identify series 2', function () {
            expect(myDTW.run(inputSeries)).to.equal("testSeries2");
        });

        it('should report costs', function () {
            expect(myDTW.getCosts()[0]).to.equal(14.621232784634294);
            expect(myDTW.getCosts()[1]).to.equal(0);
        });

        it('should run against one label', function () {
            expect(myDTW.run(inputSeries, "testSeries2")).to.equal(0);
        });

        it('should report new costs');
        // it('should report new costs', function () {
        //     expect(myDTW2.getCosts(testSeries)[0]).to.equal(0);
        // });
        it('should test reset');
    });
});

describe('RapidLib Signal Processing', function () {
    let myStream = new rapidMix.StreamBuffer;
    let myStream10 = new rapidMix.StreamBuffer(10);
    it('should create a new StreamBuffer object', function () {
        expect(myStream).to.be.an.instanceof(rapidMix.StreamBuffer);
        expect(myStream10).to.be.instanceof(rapidMix.StreamBuffer);
    });

    myStream.reset();
    myStream.push(3)
    myStream.push(3)
    myStream.push(-10)
    myStream.push(3);
    myStream.push(29);
    it('velocity should be 1', function () {
        expect(myStream.velocity()).to.equal(26);
    });

    let accelStream = new rapidMix.StreamBuffer;
    accelStream.push(-10);
    accelStream.push(1);
    accelStream.push(1);
    accelStream.push(11);
    it('acceleration should be 10', function () {
        expect(accelStream.acceleration()).to.equal(10);
    });

    let zeroStream = new rapidMix.StreamBuffer(7);
    zeroStream.push(-.5);
    zeroStream.push(0.707);
    zeroStream.push(0.68);
    zeroStream.push(1);
    zeroStream.push(0);
    zeroStream.push(-1);
    zeroStream.push(0);
    zeroStream.push(1);
    zeroStream.push(-1);
    zeroStream.push(0.01);

    it('zeroCrossings should be 4', function () {
        expect(zeroStream.numZeroCrossings()).to.equal(4);
    });

    describe('when streaming to statStream', function () {
        let statStream = new rapidMix.StreamBuffer(5);
        statStream.push(1.1);
        statStream.push(2.2);
        statStream.push(3.14);
        statStream.push(-4.3);
        statStream.push(33.9);
        it('the max should be 33.9', function () {
            expect(statStream.maximum()).to.equal(33.9);
        });
        it('the min should be -4.3', function () {
            expect(statStream.minimum()).to.equal(-4.3);
        });
        it('should sum to 36.04', function () {
            expect(statStream.sum()).to.equal(36.04);
        });
        it('the mean should be 7.208', function () {
            expect(statStream.mean()).to.equal(7.208);
        });
        it('the standard deviation should be 13.592889906123716', function () {
            expect(statStream.standardDeviation()).to.equal(13.592889906123716);
        });
        it('the should rms be 15.385770048977072', function () {
            expect(statStream.rms()).to.equal(15.385770048977072);
        });
    });
    describe('when streaming to velStream', function () {
        let velStream = new rapidMix.StreamBuffer(5);
        velStream.push(1.1);
        velStream.push(2.2);
        velStream.push(3.14);
        velStream.push(-4.3);
        velStream.push(33.9);
        it('check minVelocity', function () {
            expect(velStream.minVelocity()).to.equal(-7.4399999999999995);
        });
        it('check maxVelocity', function () {
            expect(velStream.maxVelocity()).to.equal(38.199999999999996);
        });
        it('check minAcceleration', function () {
            expect(velStream.minAcceleration()).to.equal(-8.379999999999999);
        });
        it('check maxAcceleration', function () {
            expect(velStream.maxAcceleration()).to.equal(45.63999999999999);
        });
    });
    describe('when streaming to bayesFilter', function () {
        let bf = new rapidMix.StreamBuffer();
        bf.bayesSetDiffusion(-2.0);
        bf.bayesSetJumpRate(-5.0);
        bf.bayesSetMVC(1.0);
        console.log("ok " + bf.bayesFilter(0.2));
    });
});
