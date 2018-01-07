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
script.async = false;
document.getElementsByTagName('head')[0].appendChild(script);
}

var ajaxDataRenderer = function(url, plot, options) {
    var ret = null;
    $.ajax({
      // have to use synchronous here, else the function 
      // will return before the data is fetched
      async: false,
      url: url,
      dataType: "json",
      success: function(data) {
		  
		for (var i = 0; i < data[0].length; i++) {data[0][i]= data[0][i] / 1000.0;}
		 	  
        ret = data;
      }
    });
    return ret;
  };
  

/* Dynamically load the CSS and JS files from the web */
var css = ["https://stuartpittaway.github.io/diyBMS/main.css", 
	"https://ajax.googleapis.com/ajax/libs/jquerymobile/1.4.5/jquery.mobile.min.css",
	"https://fonts.googleapis.com/css?family=Open+Sans:300,400,700",
	"https://cdnjs.cloudflare.com/ajax/libs/jqPlot/1.0.9/jquery.jqplot.min.css"];
css.forEach(addStylesheet);

var script = document.createElement('script'); 
document.head.appendChild(script);
script.type = 'text/javascript';
script.async = false;
script.src = "https://ajax.googleapis.com/ajax/libs/jquery/1.11.3/jquery.min.js";

script.onload = function(){
	//This fires after JQUERY has loaded
	console.log('JQUERY Ready');

	$( document ).on( "mobileinit", function() {	
		console.log('mobileinit');
		
		$(".ui-page-active").attr('id', 'main').append('<div data-role="header"><h1>DIY BMS Management Console</h1></div><div role="main"  data-role="ui-content"><div id="chart1"></div></div><div data-role="footer"><a href="https://stuartpittaway.github.io/diyBMS/">github</a></div>');
				
		
			$('#main').on( 'pageshow', function(event){
				console.log('pageshow');
				$.jqplot.config.enablePlugins = true;
				
				//var jsonurl = "http://192.168.0.35/celljson";
				var jsonurl = "./celljson";
				
				var plot1=$.jqplot('chart1',jsonurl,{
    title: "Cell Voltages",
    dataRenderer: ajaxDataRenderer,
    //dataRendererOptions: { unusedOptionalUrl: jsonurl },	
	axes:{xaxis:{label:'Cell module', renderer: $.jqplot.CategoryAxisRenderer }
	,yaxis:{ label:'Voltage',syncTicks:true, min: 2.5, max: 4.5, numberTicks:16}
	,y2axis:{label:'Temperature', min:0 ,syncTicks:true, min: 0, max: 1024, numberTicks:16}

	}//end axes
	,
     highlighter: {show:false}
	 ,series : [{		 	
			renderer:$.jqplot.BarRenderer,
			pointLabels:{show:true},showMarker:true, highlightMouseOver: false,
			rendererOptions:{ barDirection: 'vertical',barMargin:12},
			
            yaxis : 'yaxis',
            label : 'dataForAxis1'
        }, {
			pointLabels:{show:false},showMarker:true, highlightMouseOver: false,
            yaxis : 'y2axis',
            label : 'dataForAxis2'
        }]
		
  });
				
			});		
	});

	//Load the other libraries
	var js = ["https://ajax.googleapis.com/ajax/libs/jquerymobile/1.4.5/jquery.mobile.min.js",
	"https://cdnjs.cloudflare.com/ajax/libs/jqPlot/1.0.9/jquery.jqplot.min.js", 
	"https://cdnjs.cloudflare.com/ajax/libs/jqPlot/1.0.9/plugins/jqplot.barRenderer.min.js",
	"https://cdnjs.cloudflare.com/ajax/libs/jqPlot/1.0.9/plugins/jqplot.highlighter.js",
	"https://cdnjs.cloudflare.com/ajax/libs/jqPlot/1.0.9/plugins/jqplot.cursor.js",
	"https://cdnjs.cloudflare.com/ajax/libs/jqPlot/1.0.9/plugins/jqplot.pointLabels.js",
	"https://cdnjs.cloudflare.com/ajax/libs/jqPlot/1.0.9/plugins/jqplot.categoryAxisRenderer.min.js"]
	js.forEach(addJavascript);
}
