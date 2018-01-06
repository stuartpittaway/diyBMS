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
		  
		for (var i = 0; i < data[0].length; i++) {data[0][i]=data[0][i] / 1000;}
		  
        ret = data;
      }
    });
    return ret;
  };
  

/* Dynamically load the CSS and JS files from the web */
var css = ["https://stuartpittaway.github.io/diyBMS/main.css", "https://ajax.googleapis.com/ajax/libs/jquerymobile/1.4.5/jquery.mobile.min.css","https://fonts.googleapis.com/css?family=Open+Sans:300,400,700","https://cdnjs.cloudflare.com/ajax/libs/jqPlot/1.0.9/jquery.jqplot.min.css"];
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
				
		//apply overrides here			
		$('body').load('https://stuartpittaway.github.io/diyBMS/homePage.html', function () {
			
			console.log('after load');
			
			$('#main').trigger('create');
			
			$('#main').on( 'pageshow',function(event){
				console.log('pageshow');
				$.jqplot.config.enablePlugins = true;
			
				/*
				var s1 = [ 3.79,3.79,3.79,3.79,3.79,3.79,3.79 ];
				var ticks = [0,1,2,3,4,5,6];
		 
				var plot1 = $.jqplot('chart1',[s1],{
					title: 'Cell Voltages',
					seriesDefaults:{renderer:$.jqplot.BarRenderer, showMarker:false,	pointLabels: { show:true } , rendererOptions: { barDirection: 'vertical', barMargin: 15,barWidth: 35}},
					axes:{ xaxis:{ label:'Cell module', renderer: $.jqplot.CategoryAxisRenderer, ticks: ticks }	,yaxis:{ label:'Voltage', min:0, max:4.5 }		}
					,highlighter: { show: false }
				}); <!-- end of plot1 -->
				*/
				
				var jsonurl = "./celljson";
				var ticks = [0,1,2,3,4,5,6,7,8,9,10,11,12,13];
				var plot1=$.jqplot('chart1',jsonurl,{
    title: "AJAX JSON Data Renderer",
	seriesDefaults:{renderer:$.jqplot.BarRenderer, showMarker:false, pointLabels:{show:true}, rendererOptions:{ barDirection: 'vertical', barMargin: 15,barWidth: 35}},
    dataRenderer: ajaxDataRenderer,
    dataRendererOptions: { unusedOptionalUrl: jsonurl },
	axes:{ xaxis:{ label:'Cell module', renderer: $.jqplot.CategoryAxisRenderer, ticks: ticks }	
	,yaxis:{ label:'Voltage', min:0 }
	,y2axis: label:'Temperature', min:0 }
	}//end axes
	 ,series : [{
            yaxis : 'yaxis',
            label : 'dataForAxis1'
        }, {
            yaxis : 'y2axis',
            label : 'dataForAxis2'
        }]
		
  });
				console.log(plot1);
			});		
		}); // end load
	});

	//Load the other libraries
	var js = ["https://ajax.googleapis.com/ajax/libs/jquerymobile/1.4.5/jquery.mobile.min.js","https://cdnjs.cloudflare.com/ajax/libs/jqPlot/1.0.9/jquery.jqplot.min.js", "https://cdnjs.cloudflare.com/ajax/libs/jqPlot/1.0.9/plugins/jqplot.barRenderer.min.js","https://cdnjs.cloudflare.com/ajax/libs/jqPlot/1.0.9/plugins/jqplot.categoryAxisRenderer.min.js"]
	js.forEach(addJavascript);
}
