// This code is based on the code credited below, but it has been heavily modified
// further by Victor Zappi

/*
\example Gui/gui-to-bela

Gui to Bela
===========

This project is a minimal example on how to send data buffers from p5.js to Bela.
The sketch creates a button and two sliders that will control start/stop, frequency and amplitude
of an oscillator object. For the buttons and sliders we use the DOM objects available in p5js
(see https://p5js.org/reference/#group-DOM)

For sending a buffer from p5.js to Bela we use the function:
```
Bela.data.sendBuffer(0, 'float', buffer);
```
where the first argument (0) is the index of the sent buffer, second argument ('float') is the type of the
data, and third argument (buffer) is the data array to be sent.
*/

// make cnv available everywhere in this file
let cnv;
// DOM elements
let fSlider, aSlider, button, p1, p2;
// buffer to send to Bela. 3 elements: two sliders + 1 button
let buffer = [0, 0, 0];
// current state of the PLAY/STOP button.
let buttonState = 1;

function setup() {
  //Create a thin canvas where to allocate the elements (this is not strictly
  //neccessary because we will use DOM elements which can be allocated directly
  //in the window)
  cnv = createCanvas(windowWidth / 5, windowHeight * 2 / 3);

  // Center the canvas itself:
  cnv.position(
    (windowWidth  - width)  / 2,
    (windowHeight - height) / 2
  );

  //Create two sliders, first to control frequency and second to control amplitude of oscillator
  //both go from 0 to 100, starting with value of 60
  fSlider = createSlider(0, 100, 60);
  aSlider = createSlider(0, 100, 50);

  //Create a button to START/STOP sound
  button = createButton("PLAY/STOP");
  //call changeButtonState when button is pressed
  button.mouseClicked(changeButtonState);

  //Text
  p1 = createP("Frequency:");
  p2 = createP("Amplitude:");

  //This will position all DOM controls relative to the canvas
  formatDOMElements();

  // Re‐layout on window size/orientation changes
  window.addEventListener("resize", windowResized);

  //Initially, we set the button to 0 (not playing)
  changeButtonState();
}

function windowResized() {
  // re-center the canvas
  cnv.position(
    (windowWidth  - width)  / 2,
    (windowHeight - height) / 2
  );
  // re-layout the DOM elements
  formatDOMElements();
}

function formatDOMElements() {
  // figure out where the canvas lives
  const x0 = cnv.elt.offsetLeft;
  const y0 = cnv.elt.offsetTop;

  // detect “mobile” (narrow) + landscape
  const isMobile    = window.matchMedia("(max-width: 800px)").matches;
  const isLandscape = windowWidth > windowHeight;

  if (isMobile && isLandscape) {
    // ───── MOBILE LANDSCAPE LAYOUT ─────

    // Place the PLAY/STOP button on the left, vertically centered
    const bSize = 100; // square so border-radius:50% => circle
    button.size(bSize, bSize);
    button.position(
      x0 - bSize - 20,             // 20px gap to left of bar
      y0 + (height - bSize) / 2    // vertical center alongside bar
    );
    button.style('font-weight','bolder');
    button.style('border', '2px solid #000');
    button.style('border-radius', '50%');
    button.style('color', 'white');

    // Stack sliders & labels to the right, at 25% / 75% of bar height
    const gap = 20;                // horizontal gap
    const sx  = x0 + width + gap;  // x-start of right column
    const fy  = y0 + height * 0.25;
    const ay  = y0 + height * 0.75;

    p1.position(sx, fy - 25);
    fSlider.position(sx, fy);
    p2.position(sx, ay - 25);
    aSlider.position(sx, ay);

  } else {
    // ───── DESKTOP & PORTRAIT LAYOUT ─────

    // Format sliders centered under the canvas:
    const sw1 = fSlider.elt.offsetWidth;
    fSlider.position(
      x0 + (width - sw1) / 2,
      y0 + height/2 + 20
    );
    const sw2 = aSlider.elt.offsetWidth;
    aSlider.position(
      x0 + (width - sw2) / 2,
      y0 + height/2 + 90
    );

    // Format START/STOP button centered above them
    const bw = 100, bh = 100;
    button.size(bw, bh);
    button.position(
      x0 + (width - bw) / 2,
      y0 + height/2 - 150
    );
    button.style('font-weight','bolder');
    button.style('border', '2px solid #000');
    button.style('border-radius', '50%');
    button.style('color', 'white');

    // Format text as paragraphs
    const tx = x0 + (width - sw1) / 2;
    p1.position(tx, y0 + height/2 - 20);
    p2.position(tx, y0 + height/2 + 50);
  }
}

function draw() {
  background(5, 5, 255, 1);

  // store values in the buffer
  buffer[0] = fSlider.value() / 100; // [0,1]
  buffer[1] = aSlider.value() / 100;
  buffer[2] = buttonState;

  // SEND BUFFER to Bela.
  LDSP.data.sendBuffer(0, 'float', buffer);
}

//Function that toggles buttonState and updates button color
function changeButtonState() {
  buttonState = 1 - buttonState;
  if (buttonState === 0) {
    button.style('background-color', color(50, 50, 255));
  } else {
    button.style('background-color', color(250, 0, 0));
  }
}
