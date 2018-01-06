
function addStylesheet(filename, index) {
var fileref=document.createElement("link")
fileref.setAttribute("rel", "stylesheet")
fileref.setAttribute("type", "text/css")
fileref.setAttribute("href", filename)
document.getElementsByTagName('head')[0].appendChild(fileref);
}

function addJavascript(filename, index) {
var script= document.createElement('script');
script.type= 'text/javascript';
script.src= filename;
document.getElementsByTagName('head')[0].appendChild(script);
}

var css = ["main.css", "https://ajax.googleapis.com/ajax/libs/jquerymobile/1.4.5/jquery.mobile.min.css","https://fonts.googleapis.com/css?family=Open+Sans:300,400,700","https://cdnjs.cloudflare.com/ajax/libs/jqPlot/1.0.9/jquery.jqplot.min.css"];
css.forEach(addStylesheet);