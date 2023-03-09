let menuOpen = false;
document.getElementById("menu-btn").addEventListener("click", (ev) => {
    document.getElementById("content-container").style.transition = "margin 500ms";
    document.getElementById("menu-btn").classList.toggle("open");
    if (menuOpen) {
        document.getElementById("sidebar-content-container").style.transitionDelay = "0ms";
        document.getElementById("sidebar-content-container").style.visibility = "hidden";
        document.getElementById("content-container").style.marginLeft = "0";
        document.getElementById("sidebar").style.width = "0";
        menuOpen = false;
    } else {
        document.getElementById("sidebar-content-container").style.transitionDelay = "500ms";
        document.getElementById("sidebar-content-container").style.visibility = "visible";
        document.getElementById("content-container").style.marginLeft = "210px";
        document.getElementById("sidebar").style.width = "210px";
        menuOpen = true;
    }
})

window.addEventListener('resize', (event) => {
    document.getElementById("content-container").style.transition = "margin 0ms";
})

document.querySelectorAll("nav .nav-item").forEach((element, key, parent) => {
    element.addEventListener('mouseenter', (event) => {
        event.target.children[0].style.visibility = "visible";
    });
    element.addEventListener('mouseleave', (event) => {
        event.target.children[0].style.visibility = "hidden";
    });
});

document.querySelector(".exit").addEventListener('click', (event) => {
    fetch('/login', { method: 'DELETE', redirect: 'follow'})
    .then(res => res.json())
    .then(json => {
        if (json.status == "ok"){
            window.location.href = "/login";
        }
    })
    .catch((err) => {
        console.error(err);
    });
})
