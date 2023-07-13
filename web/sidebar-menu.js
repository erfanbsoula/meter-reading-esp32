let menuOpen = false;
let sidebar = document.getElementById("sidebar");
let sidebarBtn = document.getElementById("header_sidebar-btn");
let mainContent = document.getElementById("content-container");
let sidebarContent = document.getElementById("sidebar_content-container");

sidebarBtn.addEventListener("click", (ev) => {
    mainContent.style.transition = "margin 500ms";
    sidebarBtn.classList.toggle("open");
    if (menuOpen) {
        sidebarContent.style.transitionDelay = "0ms";
        sidebarContent.style.visibility = "hidden";
        mainContent.style.marginLeft = "0";
        sidebar.style.width = "0";
        menuOpen = false;
    } else {
        sidebarContent.style.transitionDelay = "500ms";
        sidebarContent.style.visibility = "visible";
        mainContent.style.marginLeft = "210px";
        sidebar.style.width = "210px";
        menuOpen = true;
    }
})

window.addEventListener('resize', (event) => {
    mainContent.style.transition = "margin 0ms";
})

document.querySelectorAll("#sidebar_nav .nav_item").forEach((element, key, parent) => {
    element.addEventListener('mouseenter', (event) => {
        event.target.children[0].style.visibility = "visible";
    });
    element.addEventListener('mouseleave', (event) => {
        event.target.children[0].style.visibility = "hidden";
    });
});

document.getElementById("sidebar_logout").addEventListener('click', (event) => {
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