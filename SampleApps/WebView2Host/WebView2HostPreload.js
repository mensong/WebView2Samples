
//专有公共函数
window.CallExtend = function(stringPluginName, stringMethodName, stringParameters){
	return chrome.webview.hostObjects.sync.sample.CallExtend(stringPluginName, stringMethodName, stringParameters);
}

window.LoadPlugins = function(stringDllPath){
	return chrome.webview.hostObjects.sync.sample.LoadPlugins(stringDllPath);
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