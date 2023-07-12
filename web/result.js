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