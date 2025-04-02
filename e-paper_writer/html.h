/*
   e-paper writer (c) juh 2025

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, version 2.

   Includes MIT licenced code by J-M-L Jackson from the Arduino Forum
   https://forum.arduino.cc/t/uploading-various-byte-streams-to-an-esp32-using-espasncwebserver/1233455

*/

/* ============================================
  Bellow is the original license for parts used from J-M-L Jackson from the Arduino Forum:
  https://forum.arduino.cc/t/uploading-various-byte-streams-to-an-esp32-using-espasncwebserver/1233455
  
  code is placed under the MIT license
  Copyright (c) 2024 J-M-L
  For the Arduino Forum : https://forum.arduino.cc/u/j-m-l

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
  ===============================================
*/


// Embedded HTML code as a raw string literal
// this will be processed for %TEMPLATES%, so we need to use %% for a single %
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>ePaper writer by juh</title>

    <style>
      canvas {
        image-rendering: crisp-edges; 
      }
    
    body {
        font-family: Arial, sans-serif;
        background-color: #f4f4f4;
        color: #333;
        padding: 20px;
        max-width: 800px;
        margin: auto;
    }

    h1 {
        color: #228;
        border-bottom: 4px solid #ddd;
        padding-bottom: 15px;
    }

    h2 {
        color: #44a;
        margin-top:30px;
        border-bottom: 2px solid #ddd;
        padding-bottom: 5px;
    }
        
    .controls-container {
        display: flex;
        align-items: center;  /* Vertically align elements */
        gap: 15px;  /* Adds spacing between elements */
        flex-wrap: wrap;  /* Prevents overflow if the screen is too small */
    }
    
    #colorBox {
        width: 40px;
        height: 30px;
        display: inline-block;
        border: 1px solid #000;
    }
    
    select, input[type="range"], button {
        flex-shrink: 0;
    }
    
    #sliderValue, #brightnessNumber, #rednessNumber {
            font-size: 16px;
            padding: 8px;
            margin: 5px 0;
            border: 1px solid #ccc;
            border-radius: 5px;
    }

    select, input, button, textarea {
        font-size: 16px;
        padding: 8px;
        margin: 5px 0;
        border: 1px solid #ccc;
        border-radius: 5px;
    }

    input[type="file"], textarea {
        width: 100%%;
    }

    button {
        background-color: #007bff;
        color: white;
        border: none;
        cursor: pointer;
        transition: 0.2s;
    }

    button:hover {
        background-color: #0056b3;
    }

    input[type="range"] {
        width: 40%%;
    }

    canvas {
        display: block;
        border: 2px solid #222;
        margin: 10px 0;
        background-color: rgba(255, 255, 255, 1.0);
    }

    hr {
        border: none;
        border-top: 4px solid #ccc;
        margin-bottom: 20px;
    }
    
    #file-list {
        background: #fff;
        padding: 10px;
        border: 1px solid #ccc;
        border-radius: 5px;
    }

    img {
        max-width: 100%%;
        height: auto;
        border: 1px solid #ddd;
        margin-top: 10px;
    }

    label {
        display: block;
        margin: 5px 0;
        cursor: pointer;
    }

    input[type="radio"] {
        margin-right: 5px;
    }
    
    label { display: block; margin: 10px 0 4px; }
    #capacityInfo, #QRcharCount {font-weight: bold; }

    table.settings-table {
      border-collapse: collapse;
      margin-top: 1em;
    }
  
    table.settings-table td {
      padding: 6px 12px;
      vertical-align: top;
    }
  
    table.settings-table td.code {
      font-family: monospace;
    }
  
    </style>
    
</head>
<body>
    <h1>ePaper writer by juh</h1>
    <nav id="toc"></nav>
    <p>Project homepage: <a href="https://github.com/ftjuh/ePaper-writer" target="_blank" rel="noopener noreferrer">https://github.com/ftjuh/ePaper-writer</a></p>
    <h2>Draw on canvas</h2>
    <div class="controls-container">
      <canvas id="canvas" width="%EPAPER_WIDTH%" height="%EPAPER_HEIGHT%">
      </canvas>
      <div>
        <h4>Keys (while mouse pointer is in canvas):</h4>
        <p>(b)lack (r)ed (w)hite</br>
        (1) to (0): line width</br>
        (d)raw mode: draw dots and lines</br>
        (f)ill mode: fill area with current color</p>
      </div>
    </div>
    <div class="controls-container">
      <div id="colorBox"></div>
      <select id="colorDropdown">
          <option value="black">Black</option>
          <option value="red">Red</option>
          <option value="white">White</option>
      </select>
      <input type="range" id="numberSlider" min="1" max="10" step="1" value="1" style="width: 150px;">
      <span id="sliderValue">1</span>
      <select id="modeDropdown">
          <option value="draw">Draw</option>
          <option value="fill">Fill</option>
      </select>
      <button onclick="saveCanvas()">Save</button>
      <button onclick="uploadCanvas()">Upload</button>
      <button onclick="clearCanvas()">Clear</button>
    </div>
    <input type="text" maxlength="50" size="50" id="saveCanvasFileName" value="file name for saving to flash memory.png">
    <hr>
    
    <h2>Enter text with markdown</h2>
    <p><b>Simple markdown</b> (no closing tags, settings remain active until changed)</p>
<table class="settings-table">
  <tbody>
    <tr>
      <td><i>Unique settings</i></td>
      <td class="code">[WHITE][BLACK][RED]</td>
      <td>Background color, needs to come first.</td>
    </tr>
    <tr>
      <td><i>Per line settings,<br>must be in front of a line</i></td>
      <td class="code">[left][center][right]</td>
      <td>Alignment</td>
    </tr>
    <tr>
      <td></td>
      <td class="code">[font#]</td>
      <td>Select font #</td>
    </tr>
    <tr>
      <td></td>
      <td class="code">[#]</td>
      <td>Line spacing #, starting below the current line</td>
    </tr>
    <tr>
      <td><i>Per character settings</i></td>
      <td class="code">[white][black][red]</td>
      <td>Text color</td>
    </tr>
    <tr>
      <td></td>
      <td class="code">[[ ]]</td>
      <td>Use double brackets for square brackets</td>
    </tr>
  </tbody>
</table>
    <textarea id="textArea" rows="8" cols="60">
[WHITE]Background color, must be here.
[center]Set horizontal alignment.
Settings persist until changed.
[5]Change spacing to 5px, default=2
[2][red]Set foreground color.
[right]Color is a per [black]c[red]h[black]a[red]r[black]a[red]c[black]t[red]e[black]r [red]setting.
[black][left][font2]This is font two
[right][font1]Font is a per line setting.
[center][font3]All per line settings go in front.
    </textarea>
    <br>
    <button onclick="uploadText()">Upload Text</button>
    <hr>

    <h2>Create QR-code</h2>
    <div class="controls-container">
      <div id="capacityInfo">Maximum capacity: —</div>
      <div id="QRcharCount">Used: 0 characters</div>
    </div>
    <textarea id="QRtext" rows="3" cols="50"></textarea>
    <button onclick="uploadQR()">Upload QR code</button>

    <div class="controls-container">
      <label for="versionSelect">Size</label>
      <select id="versionSelect"></select>
      <label for="eccSelect">Error Correction</label>
      <select id="eccSelect">
          <option value="L">Low</option>
          <option value="M">Medium</option>
          <option value="Q">Quartile</option>
          <option value="H">High</option>
      </select>

      <label for="modeSelect"><a href="https://github.com/ricmoo/QRCode/tree/master?tab=readme-ov-file#what-is-version-error-correction-and-mode" target="_blank" rel="noopener noreferrer">Mode</a>:</label>
      <select id="modeSelect" disabled>
          <option value="Numeric">Numeric</option>
          <option value="Alphanumeric">Alphanumeric</option>
          <option value="Byte">Byte</option>
      </select>
      <label for="QRpixelSize">Pixel size</label>
      <input id="QRpixelSize" type="number" value="1" min="1" max="9" step="1" maxlength="1" style="width: 4ch;">
    </div>

    <hr>

    <h2>Save an image to flash</h2>
    <p>Use the file manager below to upload saved images to your e-paper.</p>
    <div class="controls-container">
      <form action="/uploadFile" method="post" enctype="multipart/form-data">
          <input type="file" name="userFileName" id="userFileName">
      </form>
      <button style="margin-left: 12px;" onclick="saveUserFile()">Save to flash</button>
    </div>
    <hr>

    <h2>Saved images in flash memory (file manager)</h2>
    <div id="freeFlash" style="font-weight: bold; margin-bottom: 10px;"></div>
    <button onclick="uploadSelectedFile()">Upload selected</button>
    <button onclick="downloadSelectedFile()">Download selected</button>
    <button onclick="deleteSelectedFile()">Delete selected</button>
    <button onclick="refreshFileList()">Refresh</button>
    <div class="controls-container">
      <div id="file-list">Loading...</div>
      <img id="displaySelectedImage" 
        src="data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg' width='%EPAPER_WIDTH%' height='%EPAPER_HEIGHT%'><rect width='%EPAPER_WIDTH%' height='%EPAPER_HEIGHT%' fill='white'/></svg>"       
        alt="Placeholder">
    </div>
    <p><b>Upload settings:</b></p>
    <div class="controls-container">
        <label for="rendering">Rendering</label>
        <select id="rendering">
          <option value="nearest">nearest</option>
          <option value="dithered">dithered</option>
        </select>
        <label for="brightness">Brightness</label>
        <input type="range" id="brightness" min="0" max="255" step="1" value="128" style="width: 128px;" oninput="document.getElementById('brightnessNumber').textContent = brightness.value">
        <span id="brightnessNumber">128</span>      
        <label for="redness">Redness</label>
        <input type="range" id="redness" min="0" max="255" step="1" value="192" style="width: 128px;" oninput="document.getElementById('rednessNumber').textContent = redness.value">
        <span id="rednessNumber">192</span>        
    </div>
    <hr>

        <script>
        var canvas, ctx, painting = false, prevX = 0, currX = 0, prevY = 0, currY = 0;
        var lineCol = "black", lineWidth = 1, drawMode = "draw";


        function initCanvas() {

            canvas = document.getElementById('canvas');
            canvas.setAttribute("tabindex", "0"); // Make canvas focusable
            ctx = canvas.getContext('2d');
            ctx.imageSmoothingEnabled = false;
            //ctx.fillStyle = 'white';
            //ctx.fillRect(0, 0, canvas.width, canvas.height);
            
            //ctx.translate(-0.5, -0.5)

            canvas.addEventListener('mousemove', function (e) {
                findxy('move', e);
            }, false);
            canvas.addEventListener('mousedown', function (e) {
                findxy('down', e);
            }, false);
            canvas.addEventListener('mouseup', function (e) {
                findxy('up', e);
            }, false);
            canvas.addEventListener('mouseout', function (e) {
                findxy('out', e);
            }, false);
            
            canvas.addEventListener("mouseenter", () => canvas.focus()); // Ensure canvas gets focus when the mouse enters it
            canvas.addEventListener("click", () => canvas.focus());
            canvas.addEventListener("keydown", (event) => {
                handleKeyPress(event.key);
            });

            document.getElementById('colorDropdown').addEventListener("change", function() {
                lineCol = this.value;
                document.getElementById("colorBox").style.backgroundColor = lineCol;
                //handleColorChange(selectedColor);
            });
    
            // Slider event listener
            document.getElementById('numberSlider').addEventListener("input", function() {
                lineWidth = this.value;
                document.getElementById("sliderValue").textContent = lineWidth;
                //handleNumberChange(selectedNumber);
            });

            document.getElementById('modeDropdown').addEventListener("change", function() {
                drawMode = this.value;
            });
    
            document.getElementById("colorBox").style.backgroundColor = lineCol;
            clearCanvas();

        }

        function handleKeyPress(key) {
            console.log("Key pressed inside canvas:", key);
            if (key === "b") {
                lineCol = "black";
            } else if (key === "r") {
                lineCol = "red";
            } else if (key === "w") {
                lineCol = "white";
            } else if (key === "1") {
                lineWidth = 1;
            } else if (key === "2") {
                lineWidth = 2;
            } else if (key === "3") {
                lineWidth = 3;
            } else if (key === "4") {
                lineWidth = 4;
            } else if (key === "5") {
                lineWidth = 5;
            } else if (key === "6") {
                lineWidth = 6;
            } else if (key === "7") {
                lineWidth = 7;
            } else if (key === "8") {
                lineWidth = 8;
            } else if (key === "9") {
                lineWidth = 9;
            } else if (key === "0") {
                lineWidth = 10;
            } else if (key === "d") {
                drawMode="draw";
            } else if (key === "f") {
                drawMode="fill";
            }
            document.getElementById("colorBox").style.backgroundColor = lineCol;
            document.getElementById("colorDropdown").value = lineCol;                
            document.getElementById("numberSlider").value = lineWidth;
            document.getElementById("sliderValue").textContent = lineWidth;
            document.getElementById("modeDropdown").value = drawMode;
        }
        
        function plot(x, y) {
            ctx.beginPath();
            ctx.fillStyle = lineCol;
            ctx.fillRect(x-1.5-(lineWidth/2), y-1-(lineWidth/2), lineWidth, lineWidth);
            ctx.closePath();
        }

        function findxy(res, e) {
            var rect = canvas.getBoundingClientRect();
            var offsetX = rect.left;
            var offsetY = rect.top;

            if (res === 'down') {
                prevX = currX;
                prevY = currY;
                currX = e.clientX - offsetX;
                currY = e.clientY - offsetY;
                if (drawMode === "draw") {
                  painting = true;
                  plot(currX, currY);
                } else if (drawMode === "fill"){
                  floodFill(Math.round(currX), Math.round(currY), lineCol);
                }                
            }
            if (res === 'up' || res === 'out') {
                painting = false;
            }
            if (res === 'move') {
                if (painting) {
                    prevX = currX;
                    prevY = currY;
                    currX = e.clientX - offsetX;
                    currY = e.clientY - offsetY;
                    drawLine(prevX, prevY, currX, currY);
                }
            }
        }

        function drawLine(x1, y1, x2, y2) {
            let dx = Math.abs(x2 - x1);
            let dy = Math.abs(y2 - y1);
            let sx = x1 < x2 ? 1 : -1;
            let sy = y1 < y2 ? 1 : -1;
            let err = dx - dy;        
            ctx.fillStyle = 'black';
            while (true) {
                plot(x1, y1); // Set the pixel
                //ctx.beginPath();
                //ctx.fillRect(x1 - 0.5, y1 - 0.5, 1, 1);
                //ctx.closePath();        
                if (x1 === x2 && y1 === y2) break;
                let e2 = 2 * err;
                if (e2 > -dy) { err -= dy; x1 += sx; }
                if (e2 < dx) { err += dx; y1 += sy; }
            }
        }

        function draw() {
            ctx.beginPath();
            ctx.moveTo(prevX, prevY);
            ctx.lineTo(currX, currY);
            switch(Math.floor(Math.random() * 2)) {
              case 0:
                ctx.strokeStyle = 'black';
                break;
              case 1:
                ctx.strokeStyle = 'red';
                break;
              case 2:
                ctx.strokeStyle = 'blue';
                break;
              case 3:
                ctx.strokeStyle = 'green';
                break;
              default:
                ctx.strokeStyle = 'yellow';
            } 
            ctx.lineWidth = 1;
            ctx.stroke();
            ctx.closePath();
        }

          function getPixel(data, x, y, width) {
              var index = (y * width + x) * 4;
              return [data[index], data[index + 1], data[index + 2], data[index + 3]];
          }

          function setPixel(data, x, y, width, color) {
              var index = (y * width + x) * 4;
              data[index] = color[0];
              data[index + 1] = color[1];
              data[index + 2] = color[2];
              data[index + 3] = color[3];
          }
          
          function colorsMatch(a, b, tolerance = 10) {
          return Math.abs(a[0] - b[0]) <= tolerance &&
                 Math.abs(a[1] - b[1]) <= tolerance &&
                 Math.abs(a[2] - b[2]) <= tolerance &&
                 Math.abs(a[3] - b[3]) <= tolerance;
          }

          function floodFill(x, y, col) {
              var imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
              var data = imageData.data;
              var width = imageData.width;
              var targetColor = getPixel(data, x, y, width);
              var fillColor;
              if (col === "black") {
                fillColor = [0, 0, 0, 255];
              } else if (col === "red") {
                fillColor = [255, 0, 0, 255]; 
              } else if (col === "white") {
                fillColor = [255, 255, 255, 255]; 
              }
              if (colorsMatch(targetColor, fillColor)) return;
              var stack = [[x, y]];
              while (stack.length) {
                  // var [px, py] = stack.pop();
                  var [px, py] = stack.shift();
                  if (!Number.isInteger(px) || !Number.isInteger(py)) {
                    console.log("px or py not integer: ", px, py)
                  }
                  
                  if (px < 0 || px >= width || py < 0 || py >= imageData.height || !colorsMatch(getPixel(data, px, py, width), targetColor)) {
                      continue;
                  }
                  setPixel(data, px, py, width, fillColor);
                  stack.push([px + 1, py]);
                  stack.push([px - 1, py]);
                  stack.push([px, py + 1]);
                  stack.push([px, py - 1]);
              }
              ctx.putImageData(imageData, 0, 0);
          }

        function clearCanvas() {
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            //ctx.fillStyle = 'white';
            ctx.fillStyle = "rgba(255, 255, 255, 1)";
            ctx.fillRect(0, 0, canvas.width, canvas.height);
        }

        function saveUserFile() {
            var fileInput = document.getElementById('userFileName');
            var file = fileInput.files[0];
            var formData = new FormData();
            formData.append('fileToSave', file);
            var xhr = new XMLHttpRequest();
            xhr.open('POST', '/saveUserFile', true);
            xhr.onload = function() {
                  if (xhr.status === 200) {
                      console.log("File save complete");
                      refreshFileList(); // ✅ Call only after successful upload
                  } else {
                      console.error("Save file failed:", xhr.status);
                  }
              };      
              xhr.onerror = function() {
                  console.error("Network error during upload.");
              };
            xhr.send(formData);
        }

        function saveCanvas() {
            const filename = document.getElementById("saveCanvasFileName").value;
            canvas.toBlob(function(blob) { // Convert canvas to Blob
              var xhr = new XMLHttpRequest();
              xhr.open('POST', '/saveCanvas?filename=' + encodeURIComponent(filename), true);
              xhr.setRequestHeader('Content-Type', 'application/octet-stream');
              xhr.onload = function() {
                  if (xhr.status === 200) {
                      console.log("Upload complete");
                      refreshFileList(); // ✅ Call only after successful upload
                  } else {
                      console.error("Upload failed:", xhr.status);
                  }
              };      
              xhr.onerror = function() {
                  console.error("Network error during upload.");
              };
              xhr.send(blob);
            }, 'image/png'); // Convert canvas to PNG
            refreshFileList();
        }
      
        function uploadCanvas() {
            canvas.toBlob(function(blob) { // Convert canvas to Blob
              var xhr = new XMLHttpRequest();
              xhr.open('POST', '/uploadCanvas', true);
              xhr.setRequestHeader('Content-Type', 'application/octet-stream');
              xhr.send(blob);
            }, 'image/png'); // Convert canvas to PNG
        }
      
        function uploadText() {
            var text = document.getElementById('textArea').value;
            var xhr = new XMLHttpRequest();
            xhr.open('POST', '/uploadText', true);
            xhr.setRequestHeader('Content-Type', 'application/octet-stream');
            xhr.send(text);
        }

        function uploadQR() {
            var text = document.getElementById('QRtext').value;
            var xhr = new XMLHttpRequest();
            xhr.open('POST', 
              '/uploadQR?version=' + encodeURIComponent(document.getElementById("versionSelect").value)
              + '&ecc=' + encodeURIComponent(document.getElementById("eccSelect").value)
              + '&mode=' + encodeURIComponent(document.getElementById("modeSelect").value)
              + '&pixelsize=' + encodeURIComponent(document.getElementById("QRpixelSize").value), true);
            xhr.setRequestHeader('Content-Type', 'application/octet-stream');
            xhr.send(text);
        }

        function updateFreeFlashDisplay() {
          fetch("/free-flash")
            .then(res => res.text())
            .then(bytes => {
              const formatted = Number(bytes).toLocaleString(); // adds grouping commas
              document.getElementById("freeFlash").textContent = `Free flash memory: ${formatted} bytes`;
            });
        }
        
        function refreshFileList() {
          fetch("/files")
              .then(response => {
                  console.log("Response received:", response);
                  if (!response.ok) {
                      throw new Error("HTTP error! Status: " + response.status);
                  }
                  return response.json();
              })
              .then(files => {
                  console.log("Parsed JSON data:", files);
                  let fileListDiv = document.getElementById("file-list");
                  fileListDiv.innerHTML = "";  // Clear previous content
      
                  if (files.length === 0) {
                      fileListDiv.innerHTML = "<p>No files found.</p>";
                      return;
                  }
      
                  let firstEntry = true;
                  files.forEach(file => {
                      let label = document.createElement("label");
                      let checkbox = document.createElement("input");
                      checkbox.name = "fileChoice"; // ← This makes them behave as a group
                      checkbox.type = "radio";
                      checkbox.value = file;
                      checkbox.checked = firstEntry; // ✅ Preselect the first one
                      firstEntry = false;
                      checkbox.addEventListener("change", displaySelectedFile); // Call function when this radio button is selected
                      label.appendChild(checkbox);
                      label.appendChild(document.createTextNode(" " + file));
                      label.appendChild(document.createElement("br"));
                      fileListDiv.appendChild(label);
                  });
                  displaySelectedFile();
              })
              .catch(error => {
                  console.error("Fetch error:", error);
                  document.getElementById("file-list").innerHTML = `<p>Error loading files: ${error.message}</p>`;
              });              
              updateFreeFlashDisplay();
        }

        function displaySelectedFile() {
            let selectedImage = document.querySelector('input[name="fileChoice"]:checked').value;
            document.getElementById("displaySelectedImage").src = "/get-image?file=" + selectedImage;
        }
        
        function uploadSelectedFile() {
            const selected = document.querySelector('input[name="fileChoice"]:checked');
            if (!selected) {
                alert("Please select a file to upload.");
                return;
            }
            const filename = selected.value;
            const rendering = document.getElementById('rendering').value;
            const brightness = document.getElementById('brightness').value;
            const redness = document.getElementById('redness').value;

            const params = new URLSearchParams({
              file: filename,
              rendering: rendering,
              brightness: brightness,
              redness: redness
            });

            fetch(`/uploadselected?${params.toString()}`, {
              method: 'PUT'
            })
            .then(response => {
                if (response.ok) {
                    alert(`File "${filename}" upload successfully.`);
                } else {
                    alert(`Failed to upload "${filename}".`);
                }
            })
            .catch(error => {
                console.error("Upload request failed:", error);
                alert("Error communicating with the ESP32.");
            });
        }

        function downloadSelectedFile() {
            const selected = document.querySelector('input[name="fileChoice"]:checked');
            if (!selected) {
                alert("Please select a file to download.");
                return;
            }
            const filename = selected.value;
            const link = document.createElement("a");
            link.href = `/downloadselected?file=${encodeURIComponent(filename)}`;
            link.download = filename;
            link.style.display = "none";
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
        }

        function deleteSelectedFile() {
            const selected = document.querySelector('input[name="fileChoice"]:checked');
            if (!selected) {
                alert("Please select a file to delete.");
                return;
            }
            const filename = selected.value;
            fetch(`/delete?file=${encodeURIComponent(filename)}`, {
                method: 'DELETE'
            })
            .then(response => {
                if (response.ok) {
                    alert(`File "${filename}" deleted successfully.`);
                    refreshFileList();
                } else {
                    alert(`Failed to delete "${filename}".`);
                }
            })
            .catch(error => {
                console.error("Delete request failed:", error);
                alert("Error communicating with the ESP32.");
            });
        }

const qrVersions = [
  { version: 1, size: 21, capacity: {
    L: { Numeric: 41, Alphanumeric: 25, Byte: 17 },
    M: { Numeric: 34, Alphanumeric: 20, Byte: 14 },
    Q: { Numeric: 27, Alphanumeric: 16, Byte: 11 },
    H: { Numeric: 17, Alphanumeric: 10, Byte: 7 },
  }},
  { version: 2, size: 25, capacity: {
    L: { Numeric: 77, Alphanumeric: 47, Byte: 32 },
    M: { Numeric: 63, Alphanumeric: 38, Byte: 26 },
    Q: { Numeric: 48, Alphanumeric: 29, Byte: 20 },
    H: { Numeric: 34, Alphanumeric: 20, Byte: 14 },
  }},
  { version: 3, size: 29, capacity: {
    L: { Numeric: 127, Alphanumeric: 77, Byte: 53 },
    M: { Numeric: 101, Alphanumeric: 61, Byte: 42 },
    Q: { Numeric: 77, Alphanumeric: 47, Byte: 32 },
    H: { Numeric: 58, Alphanumeric: 35, Byte: 24 },
  }},
  { version: 4, size: 33, capacity: {
    L: { Numeric: 187, Alphanumeric: 114, Byte: 78 },
    M: { Numeric: 149, Alphanumeric: 90, Byte: 62 },
    Q: { Numeric: 111, Alphanumeric: 67, Byte: 46 },
    H: { Numeric: 82, Alphanumeric: 50, Byte: 34 },
  }},
  { version: 5, size: 37, capacity: {
    L: { Numeric: 255, Alphanumeric: 154, Byte: 106 },
    M: { Numeric: 202, Alphanumeric: 122, Byte: 84 },
    Q: { Numeric: 144, Alphanumeric: 87, Byte: 60 },
    H: { Numeric: 106, Alphanumeric: 64, Byte: 44 },
  }},
  { version: 6, size: 41, capacity: {
    L: { Numeric: 322, Alphanumeric: 195, Byte: 134 },
    M: { Numeric: 255, Alphanumeric: 154, Byte: 106 },
    Q: { Numeric: 178, Alphanumeric: 108, Byte: 74 },
    H: { Numeric: 139, Alphanumeric: 84, Byte: 58 },
  }},
  { version: 7, size: 45, capacity: {
    L: { Numeric: 370, Alphanumeric: 224, Byte: 154 },
    M: { Numeric: 293, Alphanumeric: 178, Byte: 122 },
    Q: { Numeric: 207, Alphanumeric: 125, Byte: 86 },
    H: { Numeric: 154, Alphanumeric: 93, Byte: 64 },
  }},
  { version: 8, size: 49, capacity: {
    L: { Numeric: 461, Alphanumeric: 279, Byte: 192 },
    M: { Numeric: 365, Alphanumeric: 221, Byte: 152 },
    Q: { Numeric: 259, Alphanumeric: 157, Byte: 108 },
    H: { Numeric: 202, Alphanumeric: 122, Byte: 84 },
  }},
  { version: 9, size: 53, capacity: {
    L: { Numeric: 552, Alphanumeric: 335, Byte: 230 },
    M: { Numeric: 432, Alphanumeric: 262, Byte: 180 },
    Q: { Numeric: 312, Alphanumeric: 189, Byte: 130 },
    H: { Numeric: 235, Alphanumeric: 143, Byte: 98 },
  }},
  { version: 10, size: 57, capacity: {
    L: { Numeric: 652, Alphanumeric: 395, Byte: 271 },
    M: { Numeric: 513, Alphanumeric: 311, Byte: 213 },
    Q: { Numeric: 364, Alphanumeric: 221, Byte: 151 },
    H: { Numeric: 288, Alphanumeric: 174, Byte: 119 },
  }},
  { version: 11, size: 61, capacity: {
    L: { Numeric: 772, Alphanumeric: 468, Byte: 321 },
    M: { Numeric: 604, Alphanumeric: 366, Byte: 251 },
    Q: { Numeric: 427, Alphanumeric: 259, Byte: 177 },
    H: { Numeric: 331, Alphanumeric: 200, Byte: 137 },
  }},
  { version: 12, size: 65, capacity: {
    L: { Numeric: 883, Alphanumeric: 535, Byte: 367 },
    M: { Numeric: 691, Alphanumeric: 419, Byte: 287 },
    Q: { Numeric: 489, Alphanumeric: 296, Byte: 203 },
    H: { Numeric: 374, Alphanumeric: 227, Byte: 155 },
  }},
  { version: 13, size: 69, capacity: {
    L: { Numeric: 1022, Alphanumeric: 619, Byte: 425 },
    M: { Numeric: 796, Alphanumeric: 483, Byte: 331 },
    Q: { Numeric: 580, Alphanumeric: 352, Byte: 241 },
    H: { Numeric: 427, Alphanumeric: 259, Byte: 177 },
  }},
  { version: 14, size: 73, capacity: {
    L: { Numeric: 1101, Alphanumeric: 667, Byte: 458 },
    M: { Numeric: 871, Alphanumeric: 528, Byte: 362 },
    Q: { Numeric: 621, Alphanumeric: 376, Byte: 258 },
    H: { Numeric: 468, Alphanumeric: 283, Byte: 194 },
  }},
  { version: 15, size: 77, capacity: {
    L: { Numeric: 1250, Alphanumeric: 758, Byte: 520 },
    M: { Numeric: 991, Alphanumeric: 600, Byte: 412 },
    Q: { Numeric: 703, Alphanumeric: 426, Byte: 292 },
    H: { Numeric: 530, Alphanumeric: 321, Byte: 220 },
  }},
  { version: 16, size: 81, capacity: {
    L: { Numeric: 1408, Alphanumeric: 854, Byte: 586 },
    M: { Numeric: 1082, Alphanumeric: 656, Byte: 450 },
    Q: { Numeric: 775, Alphanumeric: 470, Byte: 322 },
    H: { Numeric: 602, Alphanumeric: 365, Byte: 250 },
  }},
  { version: 17, size: 85, capacity: {
    L: { Numeric: 1548, Alphanumeric: 938, Byte: 644 },
    M: { Numeric: 1212, Alphanumeric: 734, Byte: 504 },
    Q: { Numeric: 876, Alphanumeric: 531, Byte: 364 },
    H: { Numeric: 674, Alphanumeric: 408, Byte: 280 },
  }},
  { version: 18, size: 89, capacity: {
    L: { Numeric: 1725, Alphanumeric: 1046, Byte: 718 },
    M: { Numeric: 1346, Alphanumeric: 816, Byte: 560 },
    Q: { Numeric: 948, Alphanumeric: 574, Byte: 394 },
    H: { Numeric: 746, Alphanumeric: 452, Byte: 310 },
  }},
  { version: 19, size: 93, capacity: {
    L: { Numeric: 1903, Alphanumeric: 1153, Byte: 792 },
    M: { Numeric: 1500, Alphanumeric: 909, Byte: 624 },
    Q: { Numeric: 1063, Alphanumeric: 644, Byte: 442 },
    H: { Numeric: 813, Alphanumeric: 493, Byte: 338 },
  }},
  { version: 20, size: 97, capacity: {
    L: { Numeric: 2061, Alphanumeric: 1249, Byte: 858 },
    M: { Numeric: 1600, Alphanumeric: 970, Byte: 666 },
    Q: { Numeric: 1159, Alphanumeric: 702, Byte: 482 },
    H: { Numeric: 919, Alphanumeric: 557, Byte: 382 },
  }},
  { version: 21, size: 101, capacity: {
    L: { Numeric: 2232, Alphanumeric: 1352, Byte: 929 },
    M: { Numeric: 1708, Alphanumeric: 1035, Byte: 711 },
    Q: { Numeric: 1224, Alphanumeric: 742, Byte: 509 },
    H: { Numeric: 969, Alphanumeric: 587, Byte: 403 },
  }},
  { version: 22, size: 105, capacity: {
    L: { Numeric: 2409, Alphanumeric: 1460, Byte: 1003 },
    M: { Numeric: 1872, Alphanumeric: 1134, Byte: 779 },
    Q: { Numeric: 1358, Alphanumeric: 823, Byte: 565 },
    H: { Numeric: 1056, Alphanumeric: 640, Byte: 439 },
  }},
  { version: 23, size: 109, capacity: {
    L: { Numeric: 2620, Alphanumeric: 1588, Byte: 1091 },
    M: { Numeric: 2059, Alphanumeric: 1248, Byte: 857 },
    Q: { Numeric: 1468, Alphanumeric: 890, Byte: 611 },
    H: { Numeric: 1108, Alphanumeric: 672, Byte: 461 },
  }},
  { version: 24, size: 113, capacity: {
    L: { Numeric: 2812, Alphanumeric: 1704, Byte: 1171 },
    M: { Numeric: 2188, Alphanumeric: 1326, Byte: 911 },
    Q: { Numeric: 1588, Alphanumeric: 963, Byte: 661 },
    H: { Numeric: 1228, Alphanumeric: 744, Byte: 511 },
  }},
  { version: 25, size: 117, capacity: {
    L: { Numeric: 3057, Alphanumeric: 1853, Byte: 1273 },
    M: { Numeric: 2395, Alphanumeric: 1451, Byte: 997 },
    Q: { Numeric: 1718, Alphanumeric: 1041, Byte: 715 },
    H: { Numeric: 1286, Alphanumeric: 779, Byte: 535 },
  }},
  { version: 26, size: 121, capacity: {
    L: { Numeric: 3283, Alphanumeric: 1990, Byte: 1367 },
    M: { Numeric: 2544, Alphanumeric: 1542, Byte: 1059 },
    Q: { Numeric: 1804, Alphanumeric: 1094, Byte: 751 },
    H: { Numeric: 1425, Alphanumeric: 864, Byte: 593 },
  }},
  { version: 27, size: 125, capacity: {
    L: { Numeric: 3517, Alphanumeric: 2132, Byte: 1465 },
    M: { Numeric: 2701, Alphanumeric: 1637, Byte: 1125 },
    Q: { Numeric: 1933, Alphanumeric: 1172, Byte: 805 },
    H: { Numeric: 1501, Alphanumeric: 910, Byte: 625 },
  }},
  { version: 28, size: 129, capacity: {
    L: { Numeric: 3669, Alphanumeric: 2223, Byte: 1528 },
    M: { Numeric: 2857, Alphanumeric: 1732, Byte: 1190 },
    Q: { Numeric: 2085, Alphanumeric: 1263, Byte: 868 },
    H: { Numeric: 1581, Alphanumeric: 958, Byte: 658 },
  }},
  { version: 29, size: 133, capacity: {
    L: { Numeric: 3909, Alphanumeric: 2369, Byte: 1628 },
    M: { Numeric: 3035, Alphanumeric: 1839, Byte: 1264 },
    Q: { Numeric: 2181, Alphanumeric: 1322, Byte: 908 },
    H: { Numeric: 1677, Alphanumeric: 1016, Byte: 698 },
  }},
  { version: 30, size: 137, capacity: {
    L: { Numeric: 4158, Alphanumeric: 2520, Byte: 1732 },
    M: { Numeric: 3289, Alphanumeric: 1994, Byte: 1370 },
    Q: { Numeric: 2358, Alphanumeric: 1429, Byte: 982 },
    H: { Numeric: 1782, Alphanumeric: 1080, Byte: 742 },
  }},
  { version: 31, size: 141, capacity: {
    L: { Numeric: 4417, Alphanumeric: 2677, Byte: 1840 },
    M: { Numeric: 3486, Alphanumeric: 2113, Byte: 1452 },
    Q: { Numeric: 2473, Alphanumeric: 1499, Byte: 1030 },
    H: { Numeric: 1897, Alphanumeric: 1150, Byte: 790 },
  }},
  { version: 32, size: 145, capacity: {
    L: { Numeric: 4686, Alphanumeric: 2840, Byte: 1952 },
    M: { Numeric: 3693, Alphanumeric: 2238, Byte: 1538 },
    Q: { Numeric: 2670, Alphanumeric: 1618, Byte: 1112 },
    H: { Numeric: 2022, Alphanumeric: 1226, Byte: 842 },
  }},
  { version: 33, size: 149, capacity: {
    L: { Numeric: 4965, Alphanumeric: 3009, Byte: 2068 },
    M: { Numeric: 3909, Alphanumeric: 2369, Byte: 1628 },
    Q: { Numeric: 2805, Alphanumeric: 1700, Byte: 1168 },
    H: { Numeric: 2157, Alphanumeric: 1307, Byte: 898 },
  }},
  { version: 34, size: 153, capacity: {
    L: { Numeric: 5253, Alphanumeric: 3183, Byte: 2188 },
    M: { Numeric: 4134, Alphanumeric: 2506, Byte: 1722 },
    Q: { Numeric: 2949, Alphanumeric: 1787, Byte: 1228 },
    H: { Numeric: 2301, Alphanumeric: 1394, Byte: 958 },
  }},
  { version: 35, size: 157, capacity: {
    L: { Numeric: 5529, Alphanumeric: 3351, Byte: 2303 },
    M: { Numeric: 4343, Alphanumeric: 2632, Byte: 1809 },
    Q: { Numeric: 3081, Alphanumeric: 1867, Byte: 1283 },
    H: { Numeric: 2361, Alphanumeric: 1431, Byte: 983 },
  }},
  { version: 36, size: 161, capacity: {
    L: { Numeric: 5836, Alphanumeric: 3537, Byte: 2431 },
    M: { Numeric: 4588, Alphanumeric: 2780, Byte: 1911 },
    Q: { Numeric: 3244, Alphanumeric: 1966, Byte: 1351 },
    H: { Numeric: 2524, Alphanumeric: 1530, Byte: 1051 },
  }},
  { version: 37, size: 165, capacity: {
    L: { Numeric: 6153, Alphanumeric: 3729, Byte: 2563 },
    M: { Numeric: 4775, Alphanumeric: 2894, Byte: 1989 },
    Q: { Numeric: 3417, Alphanumeric: 2071, Byte: 1423 },
    H: { Numeric: 2625, Alphanumeric: 1591, Byte: 1093 },
  }},
  { version: 38, size: 169, capacity: {
    L: { Numeric: 6479, Alphanumeric: 3927, Byte: 2699 },
    M: { Numeric: 5039, Alphanumeric: 3054, Byte: 2099 },
    Q: { Numeric: 3599, Alphanumeric: 2181, Byte: 1499 },
    H: { Numeric: 2735, Alphanumeric: 1658, Byte: 1139 },
  }},
  { version: 39, size: 173, capacity: {
    L: { Numeric: 6743, Alphanumeric: 4087, Byte: 2809 },
    M: { Numeric: 5313, Alphanumeric: 3220, Byte: 2213 },
    Q: { Numeric: 3791, Alphanumeric: 2298, Byte: 1579 },
    H: { Numeric: 2927, Alphanumeric: 1774, Byte: 1219 },
  }},
  { version: 40, size: 177, capacity: {
    L: { Numeric: 7089, Alphanumeric: 4296, Byte: 2953 },
    M: { Numeric: 5596, Alphanumeric: 3391, Byte: 2331 },
    Q: { Numeric: 3993, Alphanumeric: 2420, Byte: 1663 },
    H: { Numeric: 3057, Alphanumeric: 1852, Byte: 1273 },
  }},
];

      const versionSelect = document.getElementById("versionSelect");
      const eccSelect = document.getElementById("eccSelect");
      const modeSelect = document.getElementById("modeSelect");
      const capacityInfo = document.getElementById("capacityInfo");

      // Populate version dropdown
      qrVersions.forEach(entry => {
          const option = document.createElement("option");
          option.value = entry.version;
          //option.textContent = `${entry.version} (${entry.size}x${entry.size})`;
          option.textContent = `${entry.size} x ${entry.size}`;
          versionSelect.appendChild(option);
      });

      let maxQRCapacity = 0;
      versionSelect.value = "1";
      
      const QRtextarea = document.getElementById("QRtext");
      const QRcharCount = document.getElementById("QRcharCount");
      const QRmodeSelect = document.getElementById("modeSelect");

      function updateQRCharCountAndMode() {
        // update char count
        const len = QRtextarea.value.length;
        QRcharCount.textContent = `Used: ${len} characters`;
        QRcharCount.style.color = (maxQRCapacity > 0 && len > maxQRCapacity) ? "red" : "#333";
        // update mode
        const text = QRtextarea.value;
        if (/^[0-9]*$/.test(text)) {
            modeSelect.value = "Numeric";
        } else if (/^[0-9A-Z $%%*+\-./:]*$/.test(text)) {
            modeSelect.value = "Alphanumeric";
        } else {
            modeSelect.value = "Byte";
        }
      }

      function updateCapacity() {
          const version = parseInt(versionSelect.value);
          const ecc = eccSelect.value;
          const mode = modeSelect.value;
          const info = qrVersions.find(v => v.version === version);

          if (info && info.capacity[ecc] && mode in info.capacity[ecc]) {
              maxQRCapacity = info.capacity[ecc][mode];
              capacityInfo.textContent = `Maximum capacity: ${maxQRCapacity} characters`;
          } else {
              maxQRCapacity = 0;
              capacityInfo.textContent = "Maximum capacity: —";
          }
          updateQRCharCountAndMode();
      }
 
      function updateQRmode() {
          const mode = modeSelect.value;
          const text = QRtextarea.value;
          // ✅ Validate text compatibility with selected mode
          const numeric = /^[0-9]*$/;
          const alphanumeric = /^[0-9A-Z $%%*+\-./:]*$/;
          let valid = true;
          if (mode === "Numeric") {
              valid = numeric.test(text);
          } else if (mode === "Alphanumeric") {
              valid = alphanumeric.test(text);
          } // Byte mode accepts everything
          if (!valid) {
              alert(`Warning: Your data contains elements which are not compatible with ${mode} mode, so the maximum capacity will be wrong if you don't remove them.`);
          } 
          updateQRCharCountAndMode();
      }
      
      versionSelect.addEventListener("change", updateCapacity);
      eccSelect.addEventListener("change", updateCapacity);
      modeSelect.addEventListener("change", updateQRmode);
      QRtextarea.addEventListener("input", updateQRCharCountAndMode);
      updateCapacity();

      function generateTOC(contentId = "mainContent", tocId = "toc") {
        const toc = document.getElementById("toc");
        const list = document.createElement("ul");
        document.body.querySelectorAll("h2").forEach((h, i) => {
          h.id ||= "h2-" + i;
          const li = document.createElement("li");
          li.innerHTML = `<a href="#${h.id}">${h.textContent}</a>`;
          list.appendChild(li);
        });
        if (list.children.length) {
          toc.appendChild(list);
        }
      }
      
      document.addEventListener("DOMContentLoaded", function() {
        generateTOC();
        initCanvas();
        clearCanvas();
        refreshFileList();
      });

    </script>

</body>
</html>

)rawliteral";
