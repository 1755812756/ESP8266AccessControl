//获取url所有参数
function getRequestAll() {
    var url = decodeURIComponent(location.search); //获取url中"?"符后的字串
    var theRequest = new Object();
    if (url.indexOf("?") != -1) {
        var str = url.substr(1);
        strs = str.split("&");
        for (var i = 0; i < strs.length; i++) {
            theRequest[strs[i].split("=")[0]] = unescape(strs[i].split("=")[1]);
        }
    }
    return theRequest;
}

// 获取url 参数
function getRequestData(name) {
    var params = decodeURI(window.location.search);
    var reg = new RegExp("(^|&)" + name + "=([^&]*)(&|$)");
    var r = params.substr(1).match(reg);
    if (r != null) {
        return unescape(r[2]);
    }
    return null;
}