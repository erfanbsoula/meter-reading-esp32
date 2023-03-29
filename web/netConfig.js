let apForm = document.getElementById("apWifiConfig");
let apErrorBox = document.getElementById('apErrorBox');
apErrorBox.style.display = "none";
let staForm = document.getElementById("staWifiConfig");
let staErrorBox = document.getElementById('staErrorBox');
staErrorBox.style.display = "none";

function displayError(message, errorBox) {
    errorBox.textContent = "error: " + message;
    errorBox.style.color = "#f06060";
    errorBox.style.display = "block";
    return false;
}

fetch('/apwifi', {method: 'GET'})
.then((response) => response.json())
.then((res) => {
    if (res.status == 0)
        return displayError("couldn't fetch current config!", apErrorBox);

    for (const [key, value] of Object.entries(res)) {
        const field = apForm.elements.namedItem(key)
        field && (field.value = value)
    }

    if (res.enableDHCP != undefined) {
        const checkBox = apForm.elements.namedItem("enableDHCP");
        checkBox.checked = Boolean(res.enableDHCP);
    }
})
.catch((error) => {
    displayError("couldn't fetch current config!", apErrorBox);
});

fetch('/stawifi', {method: 'GET'})
.then((response) => response.json())
.then((res) => {
    if (res.status == 0)
        return displayError("couldn't fetch current config!", staErrorBox);

    for (const [key, value] of Object.entries(res)) {
        const field = staForm.elements.namedItem(key)
        field && (field.value = value)
    }

    if (res.enableDHCP != undefined) {
        const checkBox = staForm.elements.namedItem("enableDHCP");
        checkBox.checked = Boolean(res.enableDHCP);
    }
})
.catch((error) => {
    displayError("couldn't fetch current config!", staErrorBox);
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

function isValidHostName(name) {
    const nameRegex = /^[a-z0-9]{3,15}$/;
    return nameRegex.test(name);
}

function isValidSSID(ssid) {
    const ssidRegex = /^[A-Za-z][A-Za-z0-9_*#@$-]{2,29}$/;
    return ssidRegex.test(ssid);
}

function isValidPassword(pass) {
    const passRegex = /^[A-Za-z0-9_.=+*#@$-]{3,30}$/;
    return passRegex.test(pass);
}

function isValidMacAddress(mac) {
    const macRegex = /^([0-9A-F]{2}[-]){5}[0-9A-F]{2}$/;
    return macRegex.test(mac);
}

function checkFormData(formData, ap) {
    let errorBox = staErrorBox;
    if (ap === true) errorBox = apErrorBox;

    if (!isValidHostName(formData.get('hostName')))
        return displayError("invalid host name!", errorBox);

    if (!isValidMacAddress(formData.get('macAddress')))
        return displayError("invalid mac address!", errorBox);

    if (!isValidIpAddress(formData.get('hostAddr')))
        return displayError("invalid host address!", errorBox);

    if (!isValidIpAddress(formData.get('subnetMask')))
        return displayError("invalid subnet mask!", errorBox);

    if (!isValidIpAddress(formData.get('defaultGateway')))
        return displayError("invalid default gateway!", errorBox);

    if (!isValidIpAddress(formData.get('primaryDns')))
        return displayError("invalid primary DNS!", errorBox);

    if (!isValidIpAddress(formData.get('secondaryDns')))
        return displayError("invalid secondary DNS!", errorBox);

    if (ap === true) {
        if (!isValidIpAddress(formData.get('minAddrRange')) ||
            !isValidIpAddress(formData.get('maxAddrRange')))
            return displayError("invalid address range!", errorBox);
    }

    if (!isValidSSID(formData.get('SSID')))
        return displayError("invalid SSID!", errorBox);

    if (!isValidPassword(formData.get('password')))
        return displayError("password is too short!", errorBox);

    return true;
}

apForm.addEventListener('submit', function(event) {
    event.preventDefault();

    let formData = new FormData(apForm);
    if (!checkFormData(formData, true)) return;
    
    let data = {};
    for (const [key, value] of formData.entries()) {
        data[key] = value;
    }
    data["enableDHCP"] = Number(
        formData.get('enableDHCP') == "on");

    fetch('/apwifi', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify(data),
    })
    .then((response) => response.json())
    .then((res) => {
        if (res.status == 1)
            apErrorBox.style.color = "green";
        else if (res.status == 0)
            apErrorBox.style.color = "#f06060";

        apErrorBox.textContent = "status: " + res.message;
        apErrorBox.style.display = "block";
    })
    .catch((error) => {
        displayError(error.message, apErrorBox)
    });
});

staForm.addEventListener('submit', function(event) {
    event.preventDefault();

    let formData = new FormData(staForm);
    if (!checkFormData(formData, false)) return;
    
    let data = {};
    for (const [key, value] of formData.entries()) {
        data[key] = value;
    }
    data["enableDHCP"] = Number(
        formData.get('enableDHCP') == "on");

    fetch('/stawifi', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify(data),
    })
    .then((response) => response.json())
    .then((res) => {
        if (res.status == 1)
            staErrorBox.style.color = "green";
        else if (res.status == 0)
            staErrorBox.style.color = "#f06060";

        staErrorBox.textContent = "status: " + res.message;
        staErrorBox.style.display = "block";
    })
    .catch((error) => {
        displayError(error.message, staErrorBox)
    });
});
