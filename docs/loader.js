var rooturl = "./";

var provisionurl = rooturl+"provision";
var jsonurl = rooturl+"celljson";
var getmoduleconfigurationurl = rooturl+"getmoduleconfig";
var getsettingsurl = rooturl+"getsettings";
var voltagecalibrationurl = rooturl+"setvoltcalib";
var temperaturecalibrationurl= rooturl+"settempcalib";
var aboveavgbalanceurl= rooturl+"aboveavgbalance";
var cancelavgbalanceurl= rooturl+"cancelavgbalance";
var ResetESPurl= rooturl+"ResetESP";
var setloadresistanceurl= rooturl+"setloadresistance";
var factoryreseturl= rooturl+"factoryreset";

var plot1;
var timer;
var moduletimer;
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
	  url: getsettingsurl,
	  dataType: "json",
	  success: function(data) {	

		var myswitch = $( "#emoncms_enabled" );
		myswitch[0].selectedIndex = data.emoncms_enabled ? 1:0;
		myswitch.slider( "refresh" );
		
		var myswitch_influxdb = $("#influxdb_enabled");
		myswitch_influxdb[0].selectedIndex = data.influxdb_enabled ? 1:0;
		myswitch_influxdb.slider("refresh");
		
		$("#emoncms_apikey").val( data.emoncms_apikey );
		$("#emoncms_host").val( data.emoncms_host );
		$("#emoncms_httpPort").val( data.emoncms_httpPort );
		$("#emoncms_node_offset").val( data.emoncms_node_offset );
		$("#emoncms_url").val( data.emoncms_url );
		$("#influxdb_host").val( data.influxdb_host );
		$("#influxdb_httpPort").val( data.influxdb_httpPort );
		$("#influxdb_database").val( data.influxdb_database );
		$("#influxdb_user").val( data.influxdb_user );
		$("#influxdb_password").val( data.influxdb_password );

		var myswitch_autobalance = $( "#autobalance_enabled" );
		myswitch_autobalance[0].selectedIndex = data.autobalance_enabled ? 1:0;
		myswitch_autobalance.slider( "refresh" );
		var myswitch_InverterMon = $( "#invertermon_enabled" );
		myswitch_InverterMon[0].selectedIndex = data.invertermon_enabled ? 1:0;
		myswitch_InverterMon.slider( "refresh" );
		$("#max_voltage").val( data.max_voltage );
		$("#balance_voltage").val( data.balance_voltage );
		$("#balance_dev").val( data.balance_dev );

		}
	});
} //end function

function refreshModules() {
	$.ajax({
	  async: true,
	  url: getmoduleconfigurationurl,
	  dataType: "json",
	  success: function(data) {	
	  	  
		if ( $( "#ct" ).length==0 ) {
			//Create table if it doesn't exist already
			$("#moduletable").empty().append("<table id='ct' data-role='table'><thead><tr><th>Module<br/>id</th><th>Current<br/>Voltage</th><th>Voltage<br/>calibration</th><th>Manual<br/>Calibration</th><th>Temperature</th><th>Bypass<br/>Status</th><th>Temp<br/>calibration</th><th>Load<br/>resistance</th></tr></thead><tbody></tbody></table>");
			$.each(data, function(){ 
				$("#ct tbody").append("<tr id='module"+this.address+"'><td >"+this.address+"</td> \
				<td id='module"+this.address+"volt' data-moduleid='"+this.address+"' data-value='' class='voltage v'>&nbsp;</td> \
				<td class='v'><input data-moduleid='"+this.address+"' class='voltcalib' size=8 type='number' step='0.001' min='1.000' max='99.999' value='"+this.voltc.toFixed(3)+"'/></td> \
				<td class='v'><input data-moduleid='"+this.address+"' class='manualreading' size=8 type='number' step='0.001' min='1.000' max='99.999' value=''/> \
				<input type='button' data-moduleid='"+this.address+"' class='manualreadingbutton' type='button' value='Go'/> \
				</td> \
				<td id='module"+this.address+"temp' class='t'>&nbsp;</td> \
				<td id='module"+this.address+"bypass' class='t'>&nbsp;</td> \
				<td class='t'><input data-moduleid='"+this.address+"' class='tempcalib' size=8 type='number' step='0.001' min='0.001' max='99.999' value='"+this.tempc.toFixed(3)+"'/></td> \
				<td><input data-moduleid='"+this.address+"' class='resistancec' size=8 type='number' step='0.1' min='1.0' max='200.000' value='"+this.resistance.toFixed(3)+"'/></td> \
				<td><input type='button' data-moduleid='"+this.address+"' class='factoryreset' type='button' value='Factory Reset'/></td> \
				</tr>");
			});

			$('.voltcalib').on("change", function (e) {
				$.post( voltagecalibrationurl, { module: $(this).data( "moduleid" ), value: $(this).val() } );
			});

			$('.tempcalib').on("change", function (e) {				
				$.post( temperaturecalibrationurl, { module: $(this).data( "moduleid" ), value: $(this).val() } );					
			});
			$('.resistancec').on("change", function (e) {				
				$.post( setloadresistanceurl, { module: $(this).data( "moduleid" ), value: $(this).val() } );					
			});
			$('.factoryreset').on("click", function (e) {				
				$.post( factoryreseturl, { module: $(this).data( "moduleid" ) } );					
			});
			
			$('.manualreadingbutton').on("click", function (e) {			
				var moduleid=$(this).data( "moduleid" );
				
				var v=parseFloat( $(".manualreading[data-moduleid='"+moduleid+"']").val());
				
				if (!isNaN(v)) {

					var currentV=parseFloat( $(".voltage[data-moduleid='"+moduleid+"']").data("value"));

					var scale=parseFloat( $(".voltcalib[data-moduleid='"+moduleid+"']").val());
				
					scale= (v/currentV) * scale;
				
					console.log("Module:",moduleid," CurrentV:",currentV," ActualV:",v," New Scale:",scale);
					
					$(".voltcalib[data-moduleid='"+moduleid+"']").val(scale.toFixed(3));
					$(".manualreading[data-moduleid='"+moduleid+"']").val("");
					
					$.post( voltagecalibrationurl, { module: moduleid, value: scale } );
				
				}
				//$.post( factoryreseturl, { module: $(this).data( "moduleid" ) } );					
			});
			
		} //end if
				
		//Update the voltage and temperature every refresh
		$.each(data, function(){ 
			$("#module"+this.address+"volt").data("value",(this.volt/1000.0).toFixed(3));		
			$("#module"+this.address+"volt").html(""+(this.volt /1000.0).toFixed(3)+"");		
			$("#module"+this.address+"temp").html(""+this.temp+"");
			$("#module"+this.address+"bypass").html(""+this.bypass+"");
		});
		
	  }
	});	
	
	moduletimer=setTimeout(refreshModules, 5000);
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
		var t=[];
		for (var i = 0; i < data[0].length; i++) {
			data[0][i]= data[0][i] / 1000.0;
			data[2][i]= data[2][i] / 1000.0;
			data[3][i]= data[3][i] / 1000.0;		
			t.push(data[0][i]+"V ["+ data[4][i]+"]");
		}
		
		if (data[0].length==0) {
			$("#nodata").show();
		} else {
			$("#nodata").hide();
				plot1=$.jqplot('chart1',data,{
				title: "Cell Voltages",
				axes:{xaxis:{label:'Cell module',renderer:$.jqplot.CategoryAxisRenderer, ticks: t }
				,yaxis:{ label:'Voltage',syncTicks:true, min: 2.0, max: 4.3, numberTicks:23, tickOptions:{formatString:'%.2f'} }
				,y2axis:{label:'Temperature',syncTicks:true,min:-25, max:100, numberTicks:23, tickOptions:{formatString:'%.2f'}}
				}//end axes
				,
				 highlighter: { show: false, showMarker:false, tooltipAxes: 'xy', yvalues: 1}
				 ,series : [
				 {		 	//Voltage
						renderer:$.jqplot.BarRenderer,
						showMarker:false, highlightMouseOver: false,
						rendererOptions:{ barDirection: 'vertical', barMargin:12},					
						yaxis : 'yaxis',
						label : 'Voltage'						
						,pointLabels:{show:false,formatString:'%.2f'}
					}, {
						//Temperature
						pointLabels:{show:false,formatString:'%.2f'},
						showMarker:false, highlightMouseOver: false,
						yaxis : 'y2axis',
						label : 'Temperature'
					}
					, {
						//Max voltage
						lineWidth: 3, color: 'green',
		markerRenderer: $.jqplot.MarkerRenderer,
		markerOptions: {
			show: true, style: 'circle', color: 'green',lineWidth: 10,size: 2,shadow:true,	
			shadowAngle: 0,	shadowOffset: 0, shadowDepth: 1,shadowAlpha: 0.07}	
			,linePattern: 'dashed', yaxis : 'yaxis',label : 'VoltMax'
			,pointLabels:{show:true,formatString:'%.2f'}
			}, 
		{ //Min voltage
		lineWidth: 3,
		color: 'orange',
		markerRenderer: $.jqplot.MarkerRenderer,
		markerOptions: {show: true,style: 'circle',color: 'orange',lineWidth: 10,size: 2,
		shadow: true,shadowAngle: 0,shadowOffset: 0,shadowDepth: 1,shadowAlpha: 0.07}	
			,linePattern: 'dashed', yaxis : 'yaxis',label : 'VoltMin'
			,pointLabels:{show:true,formatString:'%.2f'}
					}
					, {//difference from average
					show:false }					
					]	
			  });
		  
	$('#chart1').height($(window).height()*0.85);

	$(window).bind('resize', function(event, ui) {
        $('#chart1').height($(window).height()*0.85);
        $('#chart1').width($(window).width()*0.90);
        plot1.replot({resetAxes:true});
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
	<div role="main" data-role="ui-content"><div id="nodata">There is no data available, please configure modules.</div> \
	<div id="chart1"></div> \
	<div id="buttons"><a href="#config" data-transition="pop" class="ui-btn ui-corner-all ui-shadow ui-btn-inline">Configure</a> <a href="#modules" data-transition="pop" class="ui-btn ui-corner-all ui-shadow ui-btn-inline">Modules</a>  \
	<a id="AboveAvgBalance" class="ui-btn ui-corner-all ui-shadow ui-btn-inline">Above Avg Balance</a>  <a id="CancelAvgBalance" class="ui-btn ui-corner-all ui-shadow ui-btn-inline">Cancel Avg Balance</a> <a id="ResetESP" class="ui-btn ui-corner-all ui-shadow ui-btn-inline">Reset Controller</a> <a id="github" class="ui-btn ui-corner-all ui-shadow ui-btn-inline" href="https://github.com/chickey/diyBMS">GitHub</a></div></div> \
	</div> \
	<div data-role="page" id="config" data-dom-cache="true"> \
	<div data-role="header"><h1>Configuration</h1></div> \
	<div role="main" data-role="ui-content"> \
	<h1>Configuration</h1> \
	\
	<h2>emonCMS Integration</h2> \
	<form id="form_emoncms" method="POST" action="'+rooturl+'setemoncms">\
	<div class="ui-field-contain"> \
	<label for="emoncms_enabled">emonCMS enabled</label> \
	<select data-role="slider" id="emoncms_enabled" name="emoncms_enabled"> \
	<option value="0">Off</option> \
	<option value="1">On</option> \
	</select> \
	</div> \
	\
	<div class="ui-field-contain"> \
	<label for="emoncms_host">Host:</label> \
	<input id="emoncms_host" name="emoncms_host" size="64" type="text" /> \
	</div> \
	\
	<div class="ui-field-contain"> \
	<label for="emoncms_httpPort">HTTP Port:</label> \
	<input id="emoncms_httpPort" name="emoncms_httpPort" size="40" type="number" /> \
	</div> \
	\
	<div class="ui-field-contain"> \
	<label for="emoncms_node_offset">Node offset:</label> \
	<input id="emoncms_node_offset" name="emoncms_node_offset" size="40" type="number" /> \
	</div> \
	<div class="ui-field-contain"> \
	<label for="emoncms_url">URI:</label> \
	<input id="emoncms_url" name="emoncms_url" size="64" type="text" /> \
	</div> \
	<div class="ui-field-contain"> \
	<label for="emoncms_apikey">API key:</label> \
	<input id="emoncms_apikey" name="emoncms_apikey" size="32" type="text" /> \
	</div> \
	<div class="ui-field-contain"> \
    <label for="submit-1"></label> \
	<h3>InfluxDB Integration</h3> \
	<label for="influxdb_enabled">InfluxDB enabled</label> \
	<select data-role="slider" id="influxdb_enabled" name="influxdb_enabled"> \
	<option value="0">Off</option> \
	<option value="1">On</option> \
	</select> \
	</div> \
	\
	<div class="ui-field-contain"> \
	<label for="influxdb_host">InfluxDB Host:</label> \
	<input id="influxdb_host" name="influxdb_host" size="64" type="text" /> \
	</div> \
	\
	<div class="ui-field-contain"> \
	<label for="influxdb_httpPort">HTTP Port:</label> \
	<input id="influxdb_httpPort" name="influxdb_httpPort" size="40" type="number" /> \
	</div> \
		\
	<div class="ui-field-contain"> \
	<label for="influxdb_database">InfluxDB Database:</label> \
	<input id="influxdb_database" name="influxdb_database" size="64" type="text" /> \
	</div> \
		\
	<div class="ui-field-contain"> \
	<label for="influxdb_user">InfluxDB Username:</label> \
	<input id="influxdb_user" name="influxdb_user" size="64" type="text" /> \
	</div> \
		\
	<div class="ui-field-contain"> \
	<label for="influxdb_password">InfluxDB Password:</label> \
	<input id="influxdb_password" name="influxdb_password" size="64" type="text" /> \
	</div> \
	\
	<div class="ui-field-contain"> \
    <label for="submit-3"></label> \
	<h4>Additional Settings</h4> \
	<label for="autobalance_enabled">Auto Balance enabled</label> \
	<select data-role="slider" id="autobalance_enabled" name="autobalance_enabled"> \
	<option value="0">Off</option> \
	<option value="1">On</option> \
	</select> \
	</div> \
	\
	<div class="ui-field-contain"> \
	<label for="max_voltage">Max Allowed Cell Voltage:</label> \
	<input id="max_voltage" name="max_voltage" size="64" type="number" min="3.00" max="4.20" step="0.01" /> \
	</div> \
	\
	<div class="ui-field-contain"> \
	<label for="balance_voltage">Voltage to Balance above:</label> \
	<input id="balance_voltage" name="balance_voltage" size="64" type="number" min="3.00" max="4.20" step="0.01" /> \
	</div> \
	\
	<div class="ui-field-contain"> \
    <label for="submit-4"></label> \
	<h4>Inverter Settings</h4> \
	<label for="invertermon_enabled">Inverter Monitoring enabled</label> \
	<select data-role="slider" id="invertermon_enabled" name="invertermon_enabled"> \
	<option value="0">Off</option> \
	<option value="1">On</option> \
	</select> \
	</div> \
	\
	<div class="ui-field-contain"> \
    <label for="submit-1"></label> \
    <button type="submit" id="submit-1" class="ui-shadow ui-btn ui-corner-all">Save</button> \
	</div>	\
	</form> \
	</div> \
	\
	<p><a href="#main" class="ui-btn ui-corner-all ui-shadow ui-btn-inline" data-rel="back">Close</a></p> \
	</div> \
	</div> \
	<div data-role="page" id="modules" data-dom-cache="true"> \
	<div data-role="header"><h1>Cell Modules</h1></div> \
	<div role="main" data-role="ui-content"> \
	<div id="moduletable"></div> \
	<p>Use the Provision button to add a new cell module to the controller.  To begin, add ONE (and only one) new module to the monitoring cable and click the Provision button.</p> \
	<p><a id="provButton" class="ui-btn ui-corner-all ui-shadow ui-btn-inline">Provision</a> <a id="syncTempCalib" class="ui-btn ui-corner-all ui-shadow ui-btn-inline">Sync Temp Calibration</a> <a href="#main" class="ui-btn ui-corner-all ui-shadow ui-btn-inline" data-rel="back">Close</a></p> \
	</div> \
	</div>');

	$( document ).on( "mobileinit", function() { 
		$.mobile.maxTransitionWidth=800;		
	});



	$(document).on("pagecontainershow", function (e, data) {		
		clearTimeout(timer);
		clearTimeout(moduletimer);
		
		if (data.toPage[0].id=='main') {
			$.jqplot.config.enablePlugins = true;				
			timer=setTimeout(refreshGraph, 150);  
		}

		if (data.toPage[0].id=='modules') {			
			moduletimer=setTimeout(refreshModules, 150);  		
		}

		if (data.toPage[0].id=='config') {			
			configtimer=setTimeout(refreshConfig, 150);  		
		}
	
		if (data.prevPage[0]!=null) {
			//Just left a page
			if (data.prevPage[0].id=='main') {
				
			}		
			if (data.prevPage[0].id=='modules') {
				$( "#ct" ).remove();
			}
			if (data.prevPage[0].id=='config') {
				
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
	
	$('#AboveAvgBalance').on("click", function (e) {
		$.ajax({
		  async: true,
		  url: aboveavgbalanceurl,
		  dataType: "json",
		  success: function(data) {	
			alert('Above Average Balancing requested');	
		  }
		});
	});

	$('#CancelAvgBalance').on("click", function (e) {
		$.ajax({
		  async: true,
		  url: cancelavgbalanceurl,
		  dataType: "json",
		  success: function(data) {	
			alert('Cancel Average Balancing requested');	
		  }
		});
	});
	
	$('#ResetESP').on("click", function (e) {
	$.ajax({
		async: true,
		url: ResetESPurl,
		dataType: "json",
		success: function(data) {	
			alert('Reset Controller requested');	
		  }
		});
	});
	
	
	$('#syncTempCalib').on("click", function (e) {
		$.ajax({
		  async: true,
		  url: getmoduleconfigurationurl,
		  dataType: "json",
		  success: function(data) {	
			var avgTempVal=0;
			$.each(data, function(){ 
				avgTempVal+=this.temp;
			});
			avgTempVal=Math.floor(avgTempVal/ data.length);

			$.each(data, function(){ 
				var v=(avgTempVal/this.temp * this.tempc).toFixed(3);
				console.log( "",this.address," current value:", this.tempc, " recommended:", v);
				$(".tempcalib[data-moduleid='"+this.address+"']").val(v);
				
				$.post( temperaturecalibrationurl, { module: this.address, value: v } );
				
			});
			

			}
		});
	});
	
$( "#form_emoncms" ).submit(function( event ) {
 	// Stop form from submitting normally
	event.preventDefault();
	
	var f = $( this );
	var url = f.attr( "action" );
	var d=f.serializeArray();

	// Send the data using post
	var posting = $.post( url,  d  );
	
	// Put the results in a div
	posting.done(function( data ) {
	  alert('ok');
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
