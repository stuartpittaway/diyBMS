//var rooturl = "http://192.168.0.35/";
var rooturl = "./";

var provisionurl = rooturl+"provision";
var jsonurl = rooturl+"celljson";
var getconfigurationurl = rooturl+"getconfiguration";
var voltagecalibrationurl = rooturl+"setvoltcalib";
var temperaturecalibrationurl= rooturl+"settempcalib";

var plot1;
var timer;
var configtimer;
				
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

function refreshConfig() {
	$.ajax({
	  async: true,
	  url: getconfigurationurl,
	  dataType: "json",
	  success: function(data) {	
	  	  
		if ( $( "#ct" ).length==0 ) {
			  //Create table if it doesnt exist already
			$("#configtable").empty();
			$("#configtable").append("<table id='ct'><thead><tr><th>Module id</th><th>Current Voltage</th><th>Voltage calibration</th><th>Temperature</th><th>Temp calibration</th></tr></thead></table>");   
			$.each(data, function(){ 
				$("#ct").append("<tr id='module"+this[0]+"'><td>"+this[0]+"</td><td></td><td><input data-moduleid='"+this[0]+"' class='voltcalib' size=8 type='number' step='0.001' min='1.000' max='99.999' value='"+this[2].toFixed(3)+"'/></td><td>&nbsp;</td><td><input data-moduleid='"+this[0]+"' class='tempcalib' size=8 type='number' step='0.001' min='0.001' max='99.999' value='"+this[4].toFixed(3)+"'/></td></tr>");
			});
					
			$('.voltcalib').on("change", function (e) {
				
				$.post( voltagecalibrationurl, { module: $(this).data( "moduleid" ), value: $(this).val() } );
				
					
			});

			$('.tempcalib').on("change", function (e) {
				
				$.post( temperaturecalibrationurl, { module: $(this).data( "moduleid" ), value: $(this).val() } );
					
			});
			
		} //end if
				
		//Update the voltage and temperature every refresh
		$.each(data, function(){ 
			$("#module"+this[0]+" > td:nth-child(2)").html(""+(this[1] /1000.0).toFixed(3)+"");		
			$("#module"+this[0]+" > td:nth-child(4)").html(""+this[3]+"");
		});
		
	  }
	});	
	
	configtimer=setTimeout(refreshConfig, 2500);
}

function refreshGraph(){ 
 
  $.ajax({
      // have to use synchronous here, else the function 
      // will return before the data is fetched
      async: true,
      url: jsonurl,
      dataType: "json",
      success: function(data) {		 
        if (plot1) plot1.destroy();

		for (var i = 0; i < data[0].length; i++) {
			data[0][i]= data[0][i] / 1000.0;
			data[2][i]= data[2][i] / 1000.0;
			data[3][i]= data[3][i] / 1000.0;
		}
		
		if (data[0].length==0) {
			$("#nodata").show();
		} else {
			$("#nodata").hide();
				plot1=$.jqplot('chart1',data,{
				title: "Cell Voltages",
				axes:{xaxis:{label:'Cell module',renderer:$.jqplot.CategoryAxisRenderer, tickOptions:{formatString:'%i'} }
				,yaxis:{ label:'Voltage',syncTicks:true, min: 2.0, max: 4.2, numberTicks:23, tickOptions:{formatString:'%.2f'} }
				,y2axis:{label:'Temperature',syncTicks:true,min:512, max:1024, numberTicks:23, tickOptions:{formatString:'%.2f'}}
				}//end axes
				,
				 highlighter: { show: true,
		  showMarker:false,
		  tooltipAxes: 'xy',
		  yvalues: 1}
				 ,series : [
				 {		 	
						renderer:$.jqplot.BarRenderer,
						pointLabels:{show:false},showMarker:false, highlightMouseOver: false,
						rendererOptions:{ barDirection: 'vertical', barMargin:12},					
						yaxis : 'yaxis',
						label : 'dataForAxis1'
					}, {
						pointLabels:{show:false},showMarker:false, highlightMouseOver: false,
						yaxis : 'y2axis',
						label : 'dataForAxis2'
					}
					, {lineWidth: 5, color: 'green',
		markerRenderer: $.jqplot.MarkerRenderer,
		markerOptions: {
			show: true,
			style: 'circle',
			color: 'green',
			lineWidth: 15,
			size: 2,
			shadow: true,
			shadowAngle: 0,
			shadowOffset: 0,
			shadowDepth: 1,
			shadowAlpha: 0.07
		}	,linePattern: 'dashed', yaxis : 'yaxis',label : 'VoltMax'}, 
		{ lineWidth: 5,
		color: 'orange',
		markerRenderer: $.jqplot.MarkerRenderer,
		markerOptions: {
			show: true,
			style: 'circle',
			color: 'orange',
			lineWidth: 15,
			size: 2,
			shadow: true,
			shadowAngle: 0,
			shadowOffset: 0,
			shadowDepth: 1,
			shadowAlpha: 0.07
		}	,linePattern: 'dashed', yaxis : 'yaxis',label : 'VoltMin'
					}
					]	
			  });
			  
		  }
	  }
    });
  
  timer=setTimeout(refreshGraph, 5000);
  }
  
/* Dynamically load the CSS and JS files from the web */
var css = ["https://stuartpittaway.github.io/diyBMS/main.css", 
	"https://ajax.googleapis.com/ajax/libs/jquerymobile/1.4.5/jquery.mobile.min.css",
	"https://fonts.googleapis.com/css?family=Open+Sans:300,400,700",
	"https://cdnjs.cloudflare.com/ajax/libs/jqPlot/1.0.9/jquery.jqplot.min.css"];
css.forEach(addStylesheet);

//Dynamically load the jquery library
var script = document.createElement('script'); 
document.head.appendChild(script);
script.type = 'text/javascript';
script.async = false;
script.src = "https://ajax.googleapis.com/ajax/libs/jquery/1.11.3/jquery.min.js";

script.onload = function(){
	//This fires after JQUERY has loaded
	console.log('JQUERY Ready');

	$("body").append('<div data-role="page" data-url="/" tabindex="0" class="ui-page ui-page-theme-a ui-page-active" id="main"> \
	<div data-role="header"><h1>DIY BMS Management Console</h1></div> \
	<div role="main" data-role="ui-content"><div id="nodata">There is no data available, please configure BMS first</div><div id="chart1"></div><div id="buttons"><a href="#config" data-transition="pop" class="ui-btn ui-corner-all ui-shadow ui-btn-inline">Configure</a> <a id="github" class="ui-btn ui-corner-all ui-shadow ui-btn-inline" href="https://github.com/stuartpittaway/diyBMS">GitHub</a></div></div> \
	</div> \
	<div data-role="page" id="config" data-dom-cache="true"> \
	<div data-role="header"><h1>Configure</h1></div> \
	<div role="main" data-role="ui-content"> \
	<p><a id="provButton" class="ui-btn ui-corner-all ui-shadow ui-btn-inline">Provision</a> Use this feature to add a new cell monitoring module to the system.  To begin, add ONE (and only one) new module to the monitoring cable and click the Provision button.</p> \
	<div id="configtable"></div> \
	<p> <a href="#main" class="ui-btn ui-corner-all ui-shadow ui-btn-inline" data-rel="back">Cancel</a></p> \
	</div> \
	</div>');

	$( document ).on( "mobileinit", function() { 
		$.mobile.maxTransitionWidth=800;		
	});

	$(document).on("pagecontainershow", function (e, data) {		
		if (data.toPage[0].id=='main') {
			$.jqplot.config.enablePlugins = true;				
			timer=setTimeout(refreshGraph, 250);  
		}

		if (data.toPage[0].id=='config') {			
			configtimer=setTimeout(refreshConfig, 250);  		
		}
	
		if (data.prevPage[0]!=null) {
			//Just left a page
			if (data.prevPage[0].id=='main') {
				clearTimeout(timer);  
			}
			
			//Just left a page
			if (data.prevPage[0].id=='config') {
				clearTimeout(configtimer);  
			}
		}
	});

	$('#provButton').on("click", function (e) {
		$.ajax({
		  async: true,
		  url: provisionurl,
		  dataType: "json",
		  success: function(data) {	
			alert('Provisioning requested');			
		  }
		});
	});
	
	
	//Load the other libraries
	var js = ["https://ajax.googleapis.com/ajax/libs/jquerymobile/1.4.5/jquery.mobile.min.js",
	"https://cdnjs.cloudflare.com/ajax/libs/jqPlot/1.0.9/jquery.jqplot.min.js", 
	"https://cdnjs.cloudflare.com/ajax/libs/jqPlot/1.0.9/plugins/jqplot.barRenderer.min.js",
	"https://cdnjs.cloudflare.com/ajax/libs/jqPlot/1.0.9/plugins/jqplot.highlighter.min.js",
	"https://cdnjs.cloudflare.com/ajax/libs/jqPlot/1.0.9/plugins/jqplot.mobile.min.js",
	"https://cdnjs.cloudflare.com/ajax/libs/jqPlot/1.0.9/plugins/jqplot.cursor.min.js",
	"https://cdnjs.cloudflare.com/ajax/libs/jqPlot/1.0.9/plugins/jqplot.pointLabels.min.js",
	"https://cdnjs.cloudflare.com/ajax/libs/jqPlot/1.0.9/plugins/jqplot.categoryAxisRenderer.min.js"]
	js.forEach(addJavascript);
}
