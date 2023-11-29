
var pluginName = LoadPlugins('Plugin.System.dll');

try{
	var value = CallExtend(pluginName, "getSharedData", "10");
} catch {
	alert("getSharedData error");
}


var value = {
	id : 10, 
	value : "mensong"
	};
var param = JSON.stringify(value);
CallExtend(pluginName, "setSharedData", param);

setTimeout(function(){
	var value = CallExtend(pluginName, "getSharedData", "10");
	alert(value);
}, 1000);