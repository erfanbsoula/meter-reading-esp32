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
        document.getElementById('description').textContent = error.message;
        document.getElementById('result').textContent = "";
        // let errorBox = element.querySelector('.error-massage');
        // errorBox.style.color = "red";
        // errorBox.textContent = error.message;
        // errorBox.style.display = "block";
    });
})


document.getElementById("button-fetch").addEventListener('click', (event) => {
    // let imageElement = document.getElementById('camera-img');

    fetch('/ai', {
        method: 'GET',
    })
    .then((response) => response.text())
    .then((txt) => {
        document.getElementById('description').textContent = "AI result:";
        document.getElementById('result').textContent = txt;
    })
    .catch((error) => {
        document.getElementById('description').textContent = error.message;
        document.getElementById('result').textContent = "";
        // let errorBox = element.querySelector('.error-massage');
        // errorBox.style.color = "red";
        // errorBox.textContent = error.message;
        // errorBox.style.display = "block";
    });
})