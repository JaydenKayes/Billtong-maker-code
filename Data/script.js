/* TODO What are these lines doing?*/
// where in the css does it say responsive?
function handleNavBar() {
    var x = document.getElementById("myTopnav");
    if (x.className === "topnav") {
        x.className += " responsive";
    }
    else {
        x.className = "topnav";
    }
} 

