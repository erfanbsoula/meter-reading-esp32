let form = document.getElementById("mqttSettings");
let errorBox = document.getElementById('error-massage');
errorBox.style.display = "none";

function displayError(message) {
    errorBox.textContent = "error: " + message;
    errorBox.style.color = "#f06060";
    errorBox.style.display = "block";
    return false;
}

fetch('/mqttConfig', {method: 'GET'})
.then((response) => response.json())
.then((res) => {
    if (res.status == 0)
        return displayError("couldn't fetch current config!");

    for (const [key, value] of Object.entries(res)) {
        const field = form.elements.namedItem(key)
        field && (field.value = value)
    }

    if (res.enableDHCP != undefined) {
        const checkBox = form.elements.namedItem("enableDHCP");
        checkBox.checked = Boolean(res.enableDHCP);
    }
})
.catch((error) => {
    displayError("couldn't fetch current config!");
});

function isValidIpAddress(ipAddress) {
    const ipRegex = /^(\d{1,3}\.){3}\d{1,3}$/;
    if (!ipRegex.test(ipAddress)) {
        return false;
    }

    const parts = ipAddress.split('.');
    for (let i = 0; i < 4; i++) {
        const part = parseInt(parts[i], 10);
        if (part < 0 || part > 255 || isNaN(part)) {
            return false;
        }
    }

    return true;
}

function isValidPortNumber(port) {
    const portNumber = parseInt(port, 10);
    return !isNaN(portNumber) && portNumber > 0 && portNumber <= 65535;
}

function isValidMqttTopic(topic) {
    const regex = /^[a-zA-Z0-9\/#+_-]+$/;
    return regex.test(topic);
}

function checkFormData(formData) {
    if (!isValidIpAddress(formData.get('serverIP')))
        return displayError("invalid server IP address!");

    if (!isValidPortNumber(formData.get('serverPort')))
        return displayError("invalid server port!");

    if (isNaN(parseInt(formData.get('timeout'), 10)))
        return displayError("invalid timeout!");

    if (!isValidMqttTopic(formData.get('statusTopic')))
        return displayError("invalid topic for status!");

    if (!isValidMqttTopic(formData.get('messageTopic')))
        return displayError("invalid topic for result!");

    return true;
}

form.addEventListener('submit', function(event) {
    event.preventDefault();

    let formData = new FormData(form);
    if (!checkFormData(formData)) return;

    let data = {};
    for (const [key, value] of formData.entries()) {
        data[key] = value;
    }
    data["mqttEnable"] = Number(
        document.getElementById('mqttEnable').checked);
    data['serverPort'] = Number(formData.get('serverPort'));
    data['timeout'] = Number(formData.get('timeout'));

    fetch('/mqttConfig', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify(data),
    })
    .then((response) => response.json())
    .then((res) => {
        if (res.status == 1)
            errorBox.style.color = "green";
        else if (res.status == 0)
            errorBox.style.color = "#f06060";

        errorBox.textContent = "status: " + res.message;
        errorBox.style.display = "block";
    })
    .catch((error) => {
        displayError(error.message)
    });
});
