function hide(element) {
    element.classList.remove("visible");
    element.classList.add("hidden");
}

function show(element) {
    element.classList.remove("hidden");
    element.classList.add("visible");
}

function changeTextAnimated(element, text, buttonText=null, input=false) {
    hide(element);

    setTimeout(function() {
        element.querySelector('p').textContent = text;
        element.querySelector('.error-massage').style.display = "none";
        document.getElementById('invert-filter').style.display = "none";

        if (buttonText != null)
            element.querySelector('button').textContent = buttonText;

        if (input)
            element.querySelector('#digit-count').style.display = "inline";
        else
            element.querySelector('#digit-count').style.display = "none";
    }, 1000)

    setTimeout(function () {
        show(element);
    }, 1100)
}

let element = document.getElementById("instruction")
element.classList.add("visible");

let progress = 0;
let digitCount = 0;
let rectSize = 0;

element.querySelector('button').addEventListener('click', (event) => {
    if (progress === 0) {
        changeTextAnimated(element, "enter the number of digits:", "next", true);
        progress += 1;
    }
    else if (progress === 1) {
        let value = parseInt(element.querySelector("#digit-count").value);
        let errorBox = element.querySelector('.error-massage');
        if (isNaN(value)) {
            errorBox.textContent = "error: please enter a valid number!";
            errorBox.style.display = "block";
            return;
        }
        if (value < 1 || 10 < value) {
            errorBox.textContent = "error: input should be in range (1 to 10)";
            errorBox.style.display = "block";
            return;
        }
        digitCount = value;
        document.querySelectorAll('svg')[0].style.display = "block";
        document.querySelectorAll('svg')[0].parentElement.parentElement.style.color = "#1F2535";
        changeTextAnimated(element, "try to place the meter digits inside the squares as good as you can.");
        setTimeout(function() {
            rectSize = createRectTrainObject(digitCount);
        }, 1200)
        progress += 1;
    }
    else if (progress === 2) {
        changeTextAnimated(element, "now, you can adjust the digits' canvas manually.");
        drawer.setConfig(digitCount, rectSize);
        drawer.enableDrawing();
        document.getElementById('rect-canvas').style.visibility = "hidden";
        document.querySelectorAll('svg')[1].style.display = "block";
        document.querySelectorAll('svg')[1].parentElement.parentElement.style.color = "#1F2535";
        progress += 1;
    }
    else if (progress === 3) {
        if (drawer.count < digitCount) {
            let errorBox = element.querySelector('.error-massage');
            errorBox.textContent = "error: please manually reconfigue all " + digitCount + " digits.";
            errorBox.style.display = "block";
            return;
        }
        document.querySelectorAll('svg')[2].style.display = "block";
        document.querySelectorAll('svg')[2].parentElement.parentElement.style.color = "#1F2535";
        drawer.disableDrawing();
        changeTextAnimated(element, "does camera image need to be inverted?");
        setTimeout(function () {
            document.getElementById('invert-filter').style.display = "block";
        }, 1000)
        progress += 1;
    }
    else if (progress === 4) {
        document.querySelectorAll('svg')[3].style.display = "block";
        document.querySelectorAll('svg')[3].parentElement.parentElement.style.color = "#1F2535";
        changeTextAnimated(element, "click SEND button to submit the configuration", "send");
        progress += 1;
    }
    else if (progress === 5) {
        postConfigsAsync();
    }
});

function findRectSize(count) {
    let rectSize = 224;
    if (count !== 1){
        rectSize = Math.floor(300/count);
    }
    return rectSize;
}

function getMagnify() {
    return 1.75;

    // if responsive:
    // let magnify = 1;
    // if (540 < window.innerWidth && window.innerWidth <= 700) {
    //     magnify = 1.5;
    // }
    // if (700 < window.innerWidth) {
    //     magnify = 1.75;
    // }
    // return magnify;
}

function createRectTrainObject(count) {
    let rectSize = findRectSize(count) * getMagnify();
    let canvas = document.querySelector('#rect-canvas');

    for (let i = 0; i < count; i++) {
        let rect = document.createElement('div');
        rect.classList.add('rect');
        rect.style.width = rectSize + 'px';
        rect.style.height = rectSize + 'px';
        canvas.appendChild(rect);
    }

    return rectSize;
}

document.getElementById('button-clear').addEventListener('click', (event) => {
    drawer.deleteAllRectangles();
})

document.getElementById('button-undo').addEventListener('click', (event) => {
    drawer.deleteLastRectangle();
})

document.getElementById('invert-filter').addEventListener('change', (event) => {
    if (event.target.checked)
        document.getElementById('camera-img').style.filter = "grayscale(100%) invert(1)";
    else
        document.getElementById('camera-img').style.filter = "grayscale(100%)";
})

function getRectanglePositions() {
    let rectanglePositions = Array(drawer.count);
    let magnify = getMagnify();
    for (let i = 0; i < drawer.count; i++) {
        rectanglePositions[i] = {
            x: Math.floor(drawer.rectangles[i].offsetLeft / magnify),
            y: Math.floor(drawer.rectangles[i].offsetTop / magnify),
            width: Math.ceil(drawer.rectangles[i].offsetWidth / magnify),
            height: Math.ceil(drawer.rectangles[i].offsetHeight / magnify)
        };
    }
    return rectanglePositions;
}

function postConfigsAsync() {
    let invert = document.getElementById('invert-filter').checked === true;
    const data = {
        digitCount: digitCount,
        invert: invert,
        rectanglePositions: getRectanglePositions()
    };

    fetch('/config', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify(data),
    })
    .then((response) => response.json())
    .then((res) => {
        let errorBox = element.querySelector('.error-massage');
        if (res.status == 1)
            errorBox.style.color = "green";
        else if (res.status == 0)
            errorBox.style.color = "red";

        errorBox.textContent = "Status: " + res.message;
        errorBox.style.display = "block";
    })
    .catch((error) => {
        let errorBox = element.querySelector('.error-massage');
        errorBox.style.color = "red";
        errorBox.textContent = error.message;
        errorBox.style.display = "block";
    });
}

// let reloadCounter = 0;
document.getElementById("button-reload").addEventListener('click', (event) => {
    let imageElement = document.getElementById('camera-img');

    fetch('/camera', {
        method: 'GET',
    })
    .then((response) => response.blob())
    .then((blob) => {
        console.log(blob);
        let fileReader = new FileReader();
        fileReader.onload = function(event) {
            console.log("Array size:", fileReader.result.byteLength)
            let mCanvas = createImageFromData(new Uint8Array(fileReader.result), 320, 240);
            imageElement.src = mCanvas.toDataURL(); // make a base64 string of the image data (the array above)
        };
        fileReader.readAsArrayBuffer(blob);
    })
    .catch((error) => {
        let errorBox = element.querySelector('.error-massage');
        errorBox.style.color = "red";
        errorBox.textContent = error.message;
        errorBox.style.display = "block";
    });
})

// rbgData - 3 bytes per pixel - alpha-channel data not used (or valid)
function createImageFromData(data, width, height)
{
    let mCanvas = document.createElement('canvas');
    mCanvas.width = width;
    mCanvas.height = height;

    let mContext = mCanvas.getContext('2d');
    let mImgData = mContext.createImageData(width, height);

    let srcIndex = 0, dstIndex = 0;
    for (let pix = 0; pix < width*height;  pix++)
    {
        let value = (data[srcIndex] - 48)*16 + (data[srcIndex+1] - 48);
        mImgData.data[dstIndex] = value;
        mImgData.data[dstIndex+1] = value;
        mImgData.data[dstIndex+2] = value;
        mImgData.data[dstIndex+3] = 255;
        srcIndex += 2;
        dstIndex += 4;
    }
    mContext.putImageData(mImgData, 0, 0);
    return mCanvas;
}
