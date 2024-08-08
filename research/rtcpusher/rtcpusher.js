function SrsError(name, message) {
    this.name = name;
    this.message = message;
    this.stack = (new Error()).stack;
}

SrsError.prototype = Object.create(Error.prototype);
SrsError.prototype.constructor = SrsError;

/**
 * 创建 WebRTC 推流发布者核心对象
 * <p></p>
 * 可以用于推流
 * @returns {{}}
 * @constructor
 */
function SrsRtcPublisherAsync() {
    let self = {};

    /**
     * getDisplayMedia 配置对象
     * @type {{audio: boolean, video: {width: {ideal: number, max: number}}}}
     */
    self.constraints = {
        audio: true,
        video: {
            width: {ideal: 320, max: 576}
        }
    };

    // @see https://github.com/rtcdn/rtcdn-draft
    // @url The WebRTC url to play with, for example:
    //      webrtc://r.ossrs.net/live/livestream
    // or specifies the API port:
    //      webrtc://r.ossrs.net:11985/live/livestream
    // or autostart the publish:
    //      webrtc://r.ossrs.net/live/livestream?autostart=true
    // or change the app from live to myapp:
    //      webrtc://r.ossrs.net:11985/myapp/livestream
    // or change the stream from livestream to mystream:
    //      webrtc://r.ossrs.net:11985/live/mystream
    // or set the api server to myapi.domain.com:
    //      webrtc://myapi.domain.com/live/livestream
    // or set the candidate(eip) of answer:
    //      webrtc://r.ossrs.net/live/livestream?candidate=39.107.238.185
    // or force to access https API:
    //      webrtc://r.ossrs.net/live/livestream?schema=https
    // or use plaintext, without SRTP:
    //      webrtc://r.ossrs.net/live/livestream?encrypt=false
    // or any other information, will pass-by in the query:
    //      webrtc://r.ossrs.net/live/livestream?vhost=xxx
    //      webrtc://r.ossrs.net/live/livestream?token=xxx
    self.publish = async function (url) {
        let conf = self.__internal.prepareUrl(url);
        // MediaStreamTrack以与所述收发器相关联
        // 这里视频轨道就传"video"，音频轨道就传"audio"
        self.pc.addTransceiver("audio", {direction: "sendonly"});
        self.pc.addTransceiver("video", {direction: "sendonly"});
        //self.pc.addTransceiver("video", {direction: "sendonly"});
        //self.pc.addTransceiver("audio", {direction: "sendonly"});

        if (!navigator.mediaDevices && window.location.protocol === 'http:' && window.location.hostname !== 'localhost') {
            throw new SrsError('HttpsRequiredError', `Please use HTTPS or localhost to publish, read https://github.com/ossrs/srs/issues/2762#issuecomment-983147576`);
        }
        // 获取流
        let stream = await navigator.mediaDevices.getDisplayMedia(self.constraints);

        // @see https://developer.mozilla.org/en-US/docs/Web/API/RTCPeerConnection/addStream#Migrating_to_addTrack
        stream.getTracks().forEach(function (track) {
            self.pc.addTrack(track);

            // 本地存一下所有的 track
            self.ontrack && self.ontrack({track: track});
        });

        let offer = await self.pc.createOffer();
        await self.pc.setLocalDescription(offer);
        let session = await new Promise(function (resolve, reject) {
            // @see https://github.com/rtcdn/rtcdn-draft
            let data = {
                api: conf.apiUrl, tid: conf.tid, streamurl: conf.streamUrl,
                clientip: null, sdp: offer.sdp
            };
            console.log("生成 offer: ", data);
            // 发请求, 进行推流
            const xhr = new XMLHttpRequest();
            xhr.onload = function () {
                if (xhr.readyState !== xhr.DONE) return;
                if (xhr.status !== 200 && xhr.status !== 201) return reject(xhr);
                const data = JSON.parse(xhr.responseText);
                console.log("Got answer: ", data);
                return data.code ? reject(xhr) : resolve(data);
            }
            xhr.open('POST', conf.apiUrl, true);
            xhr.setRequestHeader('Content-type', 'application/json');
            xhr.send(JSON.stringify(data));
        });
        // 设置远程返回的 sdp 为 remoteDescription
        await self.pc.setRemoteDescription(
            new RTCSessionDescription({type: 'answer', sdp: session.sdp})
        );
        session.simulator = conf.schema + '//' + conf.urlObject.server + ':' + conf.port + '/rtc/v1/nack/';
        console.log(session.simulator) // http://192.168.91.130:1985/rtc/v1/nack/
        // {"code":0,"server":"vid-5608q0o","service":"7p903005","pid":"8180","sdp":"v=0\r\no=SRS/5.0.170(Bee) 140272292936912 2 IN IP4 0.0.0.0\r\ns=SRSPublishSession\r\nt=0 0\r\na=ice-lite\r\na=group:BUNDLE 0 1\r\na=msid-semantic: WMS live/livestream\r\nm=audio 9 UDP/TLS/RTP/SAVPF 111\r\nc=IN IP4 0.0.0.0\r\na=ice-ufrag:146d99kn\r\na=ice-pwd:20f3w5n7e999h759aq564xhc2676619i\r\na=fingerprint:sha-256 38:C9:60:BC:51:AF:D3:25:7C:1E:6B:19:DE:FE:17:3F:15:CE:65:42:67:98:7A:26:AE:88:0A:4C:69:EC:D2:B9\r\na=setup:passive\r\na=mid:0\r\na=extmap:3 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01\r\na=recvonly\r\na=rtcp-mux\r\na=rtcp-rsize\r\na=rtpmap:111 opus/48000/2\r\na=rtcp-fb:111 transport-cc\r\na=fmtp:111 minptime=10;useinbandfec=1\r\na=candidate:0 1 udp 2130706431 192.168.91.130 8000 typ host generation 0\r\nm=video 9 UDP/TLS/RTP/SAVPF 106 116\r\nc=IN IP4 0.0.0.0\r\na=ice-ufrag:146d99kn\r\na=ice-pwd:20f3w5n7e999h759aq564xhc2676619i\r\na=fingerprint:sha-256 38:C9:60:BC:51:AF:D3:25:7C:1E:6B:19:DE:FE:17:3F:15:CE:65:42:67:98:7A:26:AE:88:0A:4C:69:EC:D2:B9\r\na=setup:passive\r\na=mid:1\r\na=extmap:3 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01\r\na=recvonly\r\na=rtcp-mux\r\na=rtcp-rsize\r\na=rtpmap:106 H264/90000\r\na=rtcp-fb:106 transport-cc\r\na=rtcp-fb:106 nack\r\na=rtcp-fb:106 nack pli\r\na=fmtp:106 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42e01f\r\na=rtpmap:116 red/90000\r\na=candidate:0 1 udp 2130706431 192.168.91.130 8000 typ host generation 0\r\n","sessionid":"146d99kn:DTM+","simulator":"http://192.168.91.130:1985/rtc/v1/nack/"}
        return session;
    };

    /**
     * 关闭 peerConnection 对象
     */
    self.close = function () {
        self.pc && self.pc.close();
        self.pc = null;
    };

    /**
     * 获取本地流时候调用
     * @param event
     */
    self.ontrack = function (event) {
        // Add track to stream of SDK.
        self.stream.addTrack(event.track);
    };

    // 内部函数
    self.__internal = {
        // 发布的url
        defaultPath: '/api/v1/rtc/publish',
        prepareUrl: function (webrtcUrl) {
            // 获取 url 基本信息, 比如说协议,ip
            let urlObject = self.__internal.parse(webrtcUrl);

            // 如果用户指定架构, use it as API schema.
            let schema = urlObject.user_query.schema;
            schema = schema ? schema + ':' : window.location.protocol;

            let port = urlObject.port || 80;
            if (schema === 'https:') {
                port = urlObject.port || 443;
            }

            // @see https://github.com/rtcdn/rtcdn-draft
            let api = urlObject.user_query.play || self.__internal.defaultPath;
            // if (api.lastIndexOf('/') !== api.length - 1) {
            //     api += '/';
            // }

            let apiUrl = schema + '//' + urlObject.server + ':' + port + api;
            for (let key in urlObject.user_query) {
                if (key !== 'api' && key !== 'play') {
                    apiUrl += '&' + key + '=' + urlObject.user_query[key];
                }
            }
            // Replace /rtc/v1/play/&k=v to /rtc/v1/play/?k=v
            apiUrl = apiUrl.replace(api + '&', api + '?');

            let streamUrl = urlObject.url;
            // {"apiUrl":"http://192.168.91.130:1985/rtc/v1/publish/","streamUrl":"webrtc://192.168.91.130/live/livestream","schema":"http:","urlObject":{"url":"webrtc://192.168.91.130/live/livestream","schema":"webrtc","server":"192.168.91.130","port":1985,"vhost":"__defaultVhost__","app":"live","stream":"livestream","user_query":{}},"port":1985,"tid":"4ea115f"}
            return {
                apiUrl: apiUrl, streamUrl: streamUrl, schema: schema, urlObject: urlObject, port: port,
                tid: Number(parseInt(new Date().getTime() * Math.random() * 100)).toString(16).slice(0, 7)
            };
        },
        // url: webrtc://192.168.91.130/live/livestream
        // 就是解析一下 url 的内容
        parse: function (url) {
            // @see: http://stackoverflow.com/questions/10469575/how-to-use-location-object-to-parse-url-without-redirecting-the-page-in-javascri
            let a = document.createElement("a");
            a.href = url.replace("rtmp://", "http://")
                .replace("webrtc://", "http://")
                .replace("rtc://", "http://");

            let vhost = a.hostname; // 192.168.91.130
            let app = a.pathname.substring(1, a.pathname.lastIndexOf("/")); // live
            let stream = a.pathname.slice(a.pathname.lastIndexOf("/") + 1); // livestream
            console.log(url, vhost, app, stream) // 192.168.91.130 live livestream
            // parse the vhost in the params of app, that srs supports.
            // 解析 SRS 支持的应用程序参数中的虚拟主机。
            app = app.replace("...vhost...", "?vhost=");
            if (app.indexOf("?") >= 0) {
                let params = app.slice(app.indexOf("?"));
                app = app.slice(0, app.indexOf("?"));

                if (params.indexOf("vhost=") > 0) {
                    vhost = params.slice(params.indexOf("vhost=") + "vhost=".length);
                    if (vhost.indexOf("&") > 0) {
                        vhost = vhost.slice(0, vhost.indexOf("&"));
                    }
                }
            }

            // when vhost equals to server, and server is ip,
            //当 vhost 等于服务器，而服务器是 ip 时
            // the vhost is __defaultVhost__
            if (a.hostname === vhost) {
                let re = /^(\d+)\.(\d+)\.(\d+)\.(\d+)$/;
                if (re.test(a.hostname)) {
                    vhost = "__defaultVhost__";
                }
            }

            // parse the schema
            let schema = "rtmp";
            if (url.indexOf("://") > 0) {
                schema = url.slice(0, url.indexOf("://")); // webrtc
            }

            let port = a.port;
            if (!port) {
                // Finger out by webrtc url, if contains http or https port, to overwrite default 1985.
                if (schema === 'webrtc' && url.indexOf(`webrtc://${a.host}:`) === 0) {
                    port = (url.indexOf(`webrtc://${a.host}:80`) === 0) ? 80 : 443;
                }

                // Guess by schema.
                if (schema === 'http') {
                    port = 80;
                } else if (schema === 'https') {
                    port = 443;
                } else if (schema === 'rtmp') {
                    port = 1935;
                }
            }

            let ret = {
                url: url,
                schema: schema,
                server: a.hostname, port: port,
                vhost: vhost, app: app, stream: stream
            };
            self.__internal.fill_query(a.search, ret);

            // For webrtc API, we use 443 if page is https, or schema specified it.
            if (!ret.port) {
                if (schema === 'webrtc' || schema === 'rtc') {
                    if (ret.user_query.schema === 'https') {
                        ret.port = 443;
                    } else if (window.location.href.indexOf('https://') === 0) {
                        ret.port = 443;
                    } else {
                        // For WebRTC, SRS use 1985 as default API port.
                        ret.port = 80;
                    }
                }
            }
            console.log(ret) //{url: 'webrtc://192.168.91.130/live/livestream', schema: 'webrtc', server: '192.168.91.130', port: 1985}
            return ret;
        },
        // 一般没啥用
        fill_query: function (query_string, obj) {
            // pure user query object.
            obj.user_query = {};

            if (query_string.length === 0) {
                // 从这里跳走了
                return;
            }

            // split again for angularjs.
            if (query_string.indexOf("?") >= 0) {
                query_string = query_string.split("?")[1];
            }

            let queries = query_string.split("&");
            for (let i = 0; i < queries.length; i++) {
                let elem = queries[i];

                let query = elem.split("=");
                obj[query[0]] = query[1];
                obj.user_query[query[0]] = query[1];
            }

            // alias domain for vhost.
            if (obj.domain) {
                obj.vhost = obj.domain;
            }
        }
    };

    // 创建点对点对象
    self.pc = new RTCPeerConnection(null);

    // 保持播放器和发布者之间的 API 一致
    // @see https://developer.mozilla.org/en-US/docs/Web/API/RTCPeerConnection/addStream#Migrating_to_addTrack
    // @see https://webrtc.org/getting-started/media-devices
    self.stream = new MediaStream();

    return self;
}

/**
 * 格式化 RTCRtpSender 的编解码器，种类（音频/视频）是可选的过滤器, 返回一串数据
 * Audio: opus, 48000HZ, channels: 2, pt: 111
 * Video: H264, 90000HZ, pt: 106
 * @param senders
 * @param kind
 * @returns {string}
 * @constructor
 */
function SrsRtcFormatSenders(senders, kind) {
    let codecs = [];
    senders.forEach(function (sender) {
        let params = sender.getParameters();
        params && params.codecs && params.codecs.forEach(function (c) {
            if (kind && sender.track && sender.track.kind !== kind) {
                return;
            }

            if (c.mimeType.indexOf('/red') > 0 || c.mimeType.indexOf('/rtx') > 0 || c.mimeType.indexOf('/fec') > 0) {
                return;
            }

            let s = '';

            s += c.mimeType.replace('audio/', '').replace('video/', '');
            s += ', ' + c.clockRate + 'HZ';
            if (sender.track && sender.track.kind === "audio") {
                s += ', channels: ' + c.channels;
            }
            s += ', pt: ' + c.payloadType;

            codecs.push(s);
        });
    });
    return codecs.join(", ");
}

let sdk = null; // Global handler to do cleanup when republishing.
let startPublish = function () {
    $('#rtc_media_player').show();

    // Close PC when user replay.
    if (sdk) {
        sdk.close();
    }
    sdk = new SrsRtcPublisherAsync();

    // User should set the stream when publish is done, @see https://webrtc.org/getting-started/media-devices
    // However SRS SDK provides a consist API like https://webrtc.org/getting-started/remote-streams
    // 设置流
    $('#rtc_media_player').prop('srcObject', sdk.stream);
    // Optional callback, SDK will add track to stream.
    // sdk.ontrack = function (event) { console.log('Got track', event); sdk.stream.addTrack(event.track); };

    // ice 连接事件, 当 ICE 收集状态（即 ICE 代理是否正在主动收集候选项）发生更改时，会发生这种情况。
    sdk.pc.onicegatheringstatechange = function (event) {
        if (sdk.pc.iceGatheringState === "complete") {
            $('#acodecs').html(SrsRtcFormatSenders(sdk.pc.getSenders(), "audio"));
            $('#vcodecs').html(SrsRtcFormatSenders(sdk.pc.getSenders(), "video"));
        }
    };

    // 示例 webrtc://192.168.91.130/live/livestream
    let url = $("#txt_url").val();

    // 开始推流
    sdk.publish(url).then(function (session) {
        $('#sessionid').html(session.sessionid);
        $('#simulator-drop').attr('href', session.simulator + '?drop=1&username=' + session.sessionid);
    }).catch(function (reason) {
        // Throw by sdk.
        if (reason instanceof SrsError) {
            if (reason.name === 'HttpsRequiredError') {
                alert(`WebRTC推流必须是HTTPS或者localhost：${reason.name} ${reason.message}`);
            } else {
                alert(`${reason.name} ${reason.message}`);
            }
        }
        if (reason instanceof DOMException) {
            if (reason.name === 'NotFoundError') {
                alert(`找不到麦克风和摄像头设备：getUserMedia ${reason.name} ${reason.message}`);
            } else if (reason.name === 'NotAllowedError') {
                alert(`你禁止了网页访问摄像头和麦克风：getUserMedia ${reason.name} ${reason.message}`);
            } else if (['AbortError', 'NotAllowedError', 'NotFoundError', 'NotReadableError', 'OverconstrainedError', 'SecurityError', 'TypeError'].includes(reason.name)) {
                alert(`getUserMedia ${reason.name} ${reason.message}`);
            }
        }

        sdk.close();
        $('#rtc_media_player').hide();
        console.error(reason);
    });
};

$('#rtc_media_player').hide();

$("#txt_url").val('webrtc://192.168.91.130/live/livestream')

$("#btn_publish").click(startPublish);
