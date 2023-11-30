
//专有公共函数

//加载插件，并返回stringPluginName
window.LoadPlugins = function(stringDllPath){
	return chrome.webview.hostObjects.sync.sample.LoadPlugins(stringDllPath);
}

window.CallExtend = function(stringPluginName, stringMethodName, stringParameters){
	if ((typeof stringParameters!='string') || stringParameters.constructor!=String)
		stringParameters = JSON.stringify(stringParameters);
	return chrome.webview.hostObjects.sync.sample.CallExtend(stringPluginName, stringMethodName, stringParameters);
}

window.LoadScript = function(stringFilePath){
	return chrome.webview.hostObjects.sync.sample.LoadScript(stringFilePath);
}

window.EvalAsync = function(stringScript){
	return chrome.webview.hostObjects.sync.sample.EvalAsync(stringScript);
}

window.GetCookies = function () {
	return chrome.webview.hostObjects.sync.sample.GetCookies();
}