// This code is based on the code credited below, but it has been modified
// further by Victor Zappi
/*
\example Gui/bela-to-gui

Bela to GUI
===========

This project is a minimal example on how to send data buffers from Bela to p5.js.
The p5.js sketch receives a buffer with only one element: a counter number.

For reading a buffer coming from Bela we use the function:
```
Bela.data.buffers[0];
```
which returns the values stored in the buffer with index 0 (defined in render.cpp)
*/

// array for changing the background color
let colors = ['red', 'blue', 'yellow', 'white', 'cyan'];

// keep a reference to the canvas so we can resize/reposition it
let cnv;

function setup() {
  // Create a full-window canvas
  cnv = createCanvas(windowWidth, windowHeight);
  // Force it to sit at the very top-left and not be inline-block
  cnv.position(0, 0);
  cnv.style('display', 'block');

  // text styling
  textFont('Courier New');
  textAlign(CENTER, CENTER);    // center text at its x,y
}

function draw() {
  // Read the latest buffer from Bela
  let counter = LDSP.data.buffers[0];

  // If Bela hasn’t sent us a number yet, counter may be NaN:
  if (!isNaN(counter) && counter >= 0) {
    background(colors[counter % colors.length]);
  } else {
    background(0);
  }

  // Draw the counter in the true center of the canvas:
  fill(100, 0, 255);
  // Choose a size that fits portrait or landscape:
  let s = min(windowWidth, windowHeight) / 4;
  textSize(s);
  text(counter, width / 2, height / 2);
}

// p5 will call this automatically when the window resizes or orientation changes
function windowResized() {
  // Resize our internal drawing buffer...
  resizeCanvas(windowWidth, windowHeight);
  // ...and make sure the DOM <canvas> still sits at 0,0 and fills it:
  cnv.position(0, 0);
}
