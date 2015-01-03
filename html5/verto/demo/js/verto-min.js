/*
 * Verto HTML5/Javascript Telephony Signaling and Control Protocol Stack for FreeSWITCH
 * Copyright (C) 2005-2014, Anthony Minessale II <anthm@freeswitch.org>
 *
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Verto HTML5/Javascript Telephony Signaling and Control Protocol Stack for FreeSWITCH
 *
 * The Initial Developer of the Original Code is
 * Anthony Minessale II <anthm@freeswitch.org>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Anthony Minessale II <anthm@freeswitch.org>
 *
 * jquery.FSRTC.js - WebRTC Glue code
 *
 */

(function($) {

    // Find the line in sdpLines that starts with |prefix|, and, if specified,
    // contains |substr| (case-insensitive search).
    function findLine(sdpLines, prefix, substr) {
        return findLineInRange(sdpLines, 0, -1, prefix, substr);
    }

    // Find the line in sdpLines[startLine...endLine - 1] that starts with |prefix|
    // and, if specified, contains |substr| (case-insensitive search).
    function findLineInRange(sdpLines, startLine, endLine, prefix, substr) {
        var realEndLine = (endLine != -1) ? endLine : sdpLines.length;
        for (var i = startLine; i < realEndLine; ++i) {
            if (sdpLines[i].indexOf(prefix) === 0) {
                if (!substr || sdpLines[i].toLowerCase().indexOf(substr.toLowerCase()) !== -1) {
                    return i;
                }
            }
        }
        return null;
    }

    // Gets the codec payload type from an a=rtpmap:X line.
    function getCodecPayloadType(sdpLine) {
        var pattern = new RegExp('a=rtpmap:(\\d+) \\w+\\/\\d+');
        var result = sdpLine.match(pattern);
        return (result && result.length == 2) ? result[1] : null;
    }

    // Returns a new m= line with the specified codec as the first one.
    function setDefaultCodec(mLine, payload) {
        var elements = mLine.split(' ');
        var newLine = [];
        var index = 0;
        for (var i = 0; i < elements.length; i++) {
            if (index === 3) { // Format of media starts from the fourth.
                newLine[index++] = payload; // Put target payload to the first.
            }
            if (elements[i] !== payload) newLine[index++] = elements[i];
        }
        return newLine.join(' ');
    }

    $.FSRTC = function(options) {
        this.options = $.extend({
            useVideo: null,
            useStereo: false,
            userData: null,
            iceServers: false,
            videoParams: {},
            audioParams: {},
            callbacks: {
                onICEComplete: function() {},
                onICE: function() {},
                onOfferSDP: function() {}
            }
        }, options);

        this.mediaData = {
            SDP: null,
            profile: {},
            candidateList: []
        };

        this.constraints = {
            optional: [{
                'DtlsSrtpKeyAgreement': 'true'
            }],
            mandatory: {
                OfferToReceiveAudio: true,
                OfferToReceiveVideo: this.options.useVideo ? true : false,
            }
        };

        if (self.options.useVideo) {
            self.options.useVideo.style.display = 'none';
        }

        setCompat();
        checkCompat();
    };

    $.FSRTC.prototype.useVideo = function(obj) {
        var self = this;

        if (obj) {
            self.options.useVideo = obj;
            self.constraints.mandatory.OfferToReceiveVideo = true;
        } else {
            self.options.useVideo = null;
            self.constraints.mandatory.OfferToReceiveVideo = false;
        }

        if (self.options.useVideo) {
            self.options.useVideo.style.display = 'none';
        }
    };

    $.FSRTC.prototype.useStereo = function(on) {
        var self = this;
        self.options.useStereo = on;
    };

    // Sets Opus in stereo if stereo is enabled, by adding the stereo=1 fmtp param.
    $.FSRTC.prototype.stereoHack = function(sdp) {
        var self = this;

        if (!self.options.useStereo) {
            return sdp;
        }

        var sdpLines = sdp.split('\r\n');

        // Find opus payload.
        var opusIndex = findLine(sdpLines, 'a=rtpmap', 'opus/48000'),
        opusPayload;
        if (opusIndex) {
            opusPayload = getCodecPayloadType(sdpLines[opusIndex]);
        }

        // Find the payload in fmtp line.
        var fmtpLineIndex = findLine(sdpLines, 'a=fmtp:' + opusPayload.toString());
        if (fmtpLineIndex === null) return sdp;

        // Append stereo=1 to fmtp line.
        sdpLines[fmtpLineIndex] = sdpLines[fmtpLineIndex].concat('; stereo=1');

        sdp = sdpLines.join('\r\n');
        return sdp;
    };

    function setCompat() {
        $.FSRTC.moz = !!navigator.mozGetUserMedia;
        //navigator.getUserMedia || (navigator.getUserMedia = navigator.mozGetUserMedia || navigator.webkitGetUserMedia || navigator.msGetUserMedia);
        if (!navigator.getUserMedia) {
            navigator.getUserMedia = navigator.mozGetUserMedia || navigator.webkitGetUserMedia || navigator.msGetUserMedia;
        }
    }

    function checkCompat() {
        if (!navigator.getUserMedia) {
            alert('This application cannot function in this browser.');
            return false;
        }
        return true;
    }

    function onStreamError(self) {
        console.log('There has been a problem retrieving the streams - did you allow access?');

    }

    function onStreamSuccess(self) {
        console.log("Stream Success");
    }

    function onICE(self, candidate) {
        self.mediaData.candidate = candidate;
        self.mediaData.candidateList.push(self.mediaData.candidate);

        doCallback(self, "onICE");
    }

    function doCallback(self, func, arg) {
        if (func in self.options.callbacks) {
            self.options.callbacks[func](self, arg);
        }
    }

    function onICEComplete(self, candidate) {
        console.log("ICE Complete");
        doCallback(self, "onICEComplete");
    }

    function onChannelError(self, e) {
        console.error("Channel Error", e);
        doCallback(self, "onError", e);
    }

    function onICESDP(self, sdp) {
        self.mediaData.SDP = self.stereoHack(sdp.sdp);
        console.log("ICE SDP");
        doCallback(self, "onICESDP");
    }

    function onAnswerSDP(self, sdp) {
        self.answer.SDP = self.stereoHack(sdp.sdp);
        console.log("ICE ANSWER SDP");
        doCallback(self, "onAnswerSDP", self.answer.SDP);
    }

    function onMessage(self, msg) {
        console.log("Message");
        doCallback(self, "onICESDP", msg);
    }

    function onRemoteStream(self, stream) {
        if (self.options.useVideo) {
            self.options.useVideo.style.display = 'block';
        }

        var element = self.options.useAudio;
        console.log("REMOTE STREAM", stream, element);

        if (typeof element.srcObject !== 'undefined') {
            element.srcObject = stream;
        } else if (typeof element.mozSrcObject !== 'undefined') {
            element.mozSrcObject = stream;
        } else if (typeof element.src !== 'undefined') {
            element.src = URL.createObjectURL(stream);
        } else {
            console.error('Error attaching stream to element.');
        }

        self.options.useAudio.play();
        self.remoteStream = stream;
    }

    function onOfferSDP(self, sdp) {
        self.mediaData.SDP = self.stereoHack(sdp.sdp);
        console.log("Offer SDP");
        doCallback(self, "onOfferSDP");
    }

    $.FSRTC.prototype.answer = function(sdp, onSuccess, onError) {
        this.peer.addAnswerSDP({
            type: "answer",
            sdp: sdp
        },
        onSuccess, onError);
    };

    $.FSRTC.prototype.stop = function() {
        var self = this;

        if (self.options.useVideo) {
            self.options.useVideo.style.display = 'none';
        }

        if (self.localStream) {
            self.localStream.stop();
            self.localStream = null;
        }

        if (self.peer) {
            console.log("stopping peer");
            self.peer.stop();
        }
    };

    $.FSRTC.prototype.createAnswer = function(sdp) {
        var self = this;
        self.type = "answer";
        self.remoteSDP = sdp;
        console.debug("inbound sdp: ", sdp);

        function onSuccess(stream) {
            self.localStream = stream;

            self.peer = RTCPeerConnection({
                type: self.type,
                attachStream: self.localStream,
                onICE: function(candidate) {
                    return onICE(self, candidate);
                },
                onICEComplete: function() {
                    return onICEComplete(self);
                },
                onRemoteStream: function(stream) {
                    return onRemoteStream(self, stream);
                },
                onICESDP: function(sdp) {
                    return onICESDP(self, sdp);
                },
                onChannelError: function(e) {
                    return onChannelError(self, e);
                },
                constraints: self.constraints,
                iceServers: self.options.iceServers,
                offerSDP: {
                    type: "offer",
                    sdp: self.remoteSDP
                }
            });

            onStreamSuccess(self);
        }

        function onError() {
            onStreamError(self);
        }

        getUserMedia({
            constraints: {
                audio: {
                    mandatory: this.options.audioParams,
                    optional: []
                },
                video: this.options.useVideo ? {
                    mandatory: this.options.videoParams,
                    optional: []
                } : null
            },
            video: this.options.useVideo ? true : false,
            onsuccess: onSuccess,
            onerror: onError
        });

    };

    $.FSRTC.prototype.call = function(profile) {
        checkCompat();

        var self = this;

        self.type = "offer";

        function onSuccess(stream) {
            self.localStream = stream;

            self.peer = RTCPeerConnection({
                type: self.type,
                attachStream: self.localStream,
                onICE: function(candidate) {
                    return onICE(self, candidate);
                },
                onICEComplete: function() {
                    return onICEComplete(self);
                },
                onRemoteStream: function(stream) {
                    return onRemoteStream(self, stream);
                },
                onOfferSDP: function(sdp) {
                    return onOfferSDP(self, sdp);
                },
                onICESDP: function(sdp) {
                    return onICESDP(self, sdp);
                },
                onChannelError: function(e) {
                    return onChannelError(self, e);
                },
                constraints: self.constraints,
                iceServers: self.options.iceServers,
            });

            onStreamSuccess(self);
        }

        function onError() {
            onStreamError(self);
        }

        getUserMedia({
            constraints: {
                audio: {
                    mandatory: this.options.audioParams,
                    optional: []
                },
                video: this.options.useVideo ? {
                    mandatory: this.options.videoParams,
                    optional: []
                } : null
            },
            video: this.options.useVideo ? true : false,
            onsuccess: onSuccess,
            onerror: onError
        });

        /*
        navigator.getUserMedia({
            video: this.options.useVideo,
            audio: true
        }, onSuccess, onError);
        */

    };

    // DERIVED from RTCPeerConnection-v1.5
    // 2013, @muazkh - github.com/muaz-khan
    // MIT License - https://www.webrtc-experiment.com/licence/
    // Documentation - https://github.com/muaz-khan/WebRTC-Experiment/tree/master/RTCPeerConnection
    window.moz = !!navigator.mozGetUserMedia;

    function RTCPeerConnection(options) {

        var w = window,
        PeerConnection = w.mozRTCPeerConnection || w.webkitRTCPeerConnection,
        SessionDescription = w.mozRTCSessionDescription || w.RTCSessionDescription,
        IceCandidate = w.mozRTCIceCandidate || w.RTCIceCandidate;

        var STUN = {
            url: !moz ? 'stun:stun.l.google.com:19302' : 'stun:23.21.150.121'
        };

        var TURN = {
            url: 'turn:homeo@turn.bistri.com:80',
            credential: 'homeo'
        };

        var iceServers = null;

        if (options.iceServers) {
            var tmp = options.iceServers;

            if (typeof(tmp) === "boolean") {
                tmp = null;
            }

            if (!(tmp && typeof(tmp) == "object" && tmp.constructor === Array)) {
                console.warn("iceServers must be an array, reverting to default ice servers");
                tmp = null;
            }

            iceServers = {
                iceServers: tmp || [STUN]
            };

            if (!moz && !tmp) {
                if (parseInt(navigator.userAgent.match(/Chrom(e|ium)\/([0-9]+)\./)[2]) >= 28) TURN = {
                    url: 'turn:turn.bistri.com:80',
                    credential: 'homeo',
                    username: 'homeo'
                };

                iceServers.iceServers = [STUN];
            }
        }

        var optional = {
            optional: []
        };

        if (!moz) {
            optional.optional = [{
                DtlsSrtpKeyAgreement: true
            },
            {
                RtpDataChannels: options.onChannelMessage ? true : false
            }];
        }

        var peer = new PeerConnection(iceServers, optional);

        openOffererChannel();
        var x = 0;

        peer.onicecandidate = function(event) {
            if (event.candidate) {
                options.onICE(event.candidate);
            } else {
                if (options.onICEComplete) {
                    options.onICEComplete();
                }

                if (options.type == "offer") {
                    if (!moz && !x && options.onICESDP) {
                        options.onICESDP(peer.localDescription);
                        //x = 1;
                        /*
                        x = 1;
                        peer.createOffer(function(sessionDescription) {
                            sessionDescription.sdp = serializeSdp(sessionDescription.sdp);
                            peer.setLocalDescription(sessionDescription);
                            if (options.onICESDP) {
                                options.onICESDP(sessionDescription);
                            }
                        }, onSdpError, constraints);
                        */
                    }
                } else {
                    if (!x && options.onICESDP) {
                        options.onICESDP(peer.localDescription);
                        //x = 1;
                        /*
                        x = 1;
                        peer.createAnswer(function(sessionDescription) {
                            sessionDescription.sdp = serializeSdp(sessionDescription.sdp);
                            peer.setLocalDescription(sessionDescription);
                            if (options.onICESDP) {
                                options.onICESDP(sessionDescription);
                            }
                        }, onSdpError, constraints);
                        */
                    }
                }
            }
        };

        // attachStream = MediaStream;
        if (options.attachStream) peer.addStream(options.attachStream);

        // attachStreams[0] = audio-stream;
        // attachStreams[1] = video-stream;
        // attachStreams[2] = screen-capturing-stream;
        if (options.attachStreams && options.attachStream.length) {
            var streams = options.attachStreams;
            for (var i = 0; i < streams.length; i++) {
                peer.addStream(streams[i]);
            }
        }

        peer.onaddstream = function(event) {
            var remoteMediaStream = event.stream;

            // onRemoteStreamEnded(MediaStream)
            remoteMediaStream.onended = function() {
                if (options.onRemoteStreamEnded) options.onRemoteStreamEnded(remoteMediaStream);
            };

            // onRemoteStream(MediaStream)
            if (options.onRemoteStream) options.onRemoteStream(remoteMediaStream);

            //console.debug('on:add:stream', remoteMediaStream);
        };

        var constraints = options.constraints || {
            optional: [],
            mandatory: {
                OfferToReceiveAudio: true,
                OfferToReceiveVideo: true
            }
        };

        // onOfferSDP(RTCSessionDescription)
        function createOffer() {
            if (!options.onOfferSDP) return;

            peer.createOffer(function(sessionDescription) {
                sessionDescription.sdp = serializeSdp(sessionDescription.sdp);
                peer.setLocalDescription(sessionDescription);
                options.onOfferSDP(sessionDescription);
                if (moz && options.onICESDP) {
                    options.onICESDP(sessionDescription);
                }
            },
            onSdpError, constraints);
        }

        // onAnswerSDP(RTCSessionDescription)
        function createAnswer() {
            if (options.type != "answer") return;

            //options.offerSDP.sdp = addStereo(options.offerSDP.sdp);
            peer.setRemoteDescription(new SessionDescription(options.offerSDP), onSdpSuccess, onSdpError);
            peer.createAnswer(function(sessionDescription) {
                sessionDescription.sdp = serializeSdp(sessionDescription.sdp);
                peer.setLocalDescription(sessionDescription);
                if (options.onAnswerSDP) {
                    options.onAnswerSDP(sessionDescription);
                }
            },
            onSdpError, constraints);
        }

        // if Mozilla Firefox & DataChannel; offer/answer will be created later
        if ((options.onChannelMessage && !moz) || !options.onChannelMessage) {
            createOffer();
            createAnswer();
        }

        // DataChannel Bandwidth
        function setBandwidth(sdp) {
            // remove existing bandwidth lines
            sdp = sdp.replace(/b=AS([^\r\n]+\r\n)/g, '');
            sdp = sdp.replace(/a=mid:data\r\n/g, 'a=mid:data\r\nb=AS:1638400\r\n');

            return sdp;
        }

        // old: FF<>Chrome interoperability management
        function getInteropSDP(sdp) {
            var chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'.split(''),
            extractedChars = '';

            function getChars() {
                extractedChars += chars[parseInt(Math.random() * 40)] || '';
                if (extractedChars.length < 40) getChars();

                return extractedChars;
            }

            // usually audio-only streaming failure occurs out of audio-specific crypto line
            // a=crypto:1 AES_CM_128_HMAC_SHA1_32 --------- kAttributeCryptoVoice
            if (options.onAnswerSDP) sdp = sdp.replace(/(a=crypto:0 AES_CM_128_HMAC_SHA1_32)(.*?)(\r\n)/g, '');

            // video-specific crypto line i.e. SHA1_80
            // a=crypto:1 AES_CM_128_HMAC_SHA1_80 --------- kAttributeCryptoVideo
            var inline = getChars() + '\r\n' + (extractedChars = '');
            sdp = sdp.indexOf('a=crypto') == -1 ? sdp.replace(/c=IN/g, 'a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:' + inline + 'c=IN') : sdp;

            return sdp;
        }

        function serializeSdp(sdp) {
            //if (!moz) sdp = setBandwidth(sdp);
            //sdp = getInteropSDP(sdp);
            //console.debug(sdp);
            return sdp;
        }

        // DataChannel management
        var channel;

        function openOffererChannel() {
            if (!options.onChannelMessage || (moz && !options.onOfferSDP)) return;

            _openOffererChannel();

            if (!moz) return;
            navigator.mozGetUserMedia({
                audio: true,
                fake: true
            },
            function(stream) {
                peer.addStream(stream);
                createOffer();
            },
            useless);
        }

        function _openOffererChannel() {
            channel = peer.createDataChannel(options.channel || 'RTCDataChannel', moz ? {} : {
                reliable: false
            });

            if (moz) channel.binaryType = 'blob';

            setChannelEvents();
        }

        function setChannelEvents() {
            channel.onmessage = function(event) {
                if (options.onChannelMessage) options.onChannelMessage(event);
            };

            channel.onopen = function() {
                if (options.onChannelOpened) options.onChannelOpened(channel);
            };
            channel.onclose = function(event) {
                if (options.onChannelClosed) options.onChannelClosed(event);

                console.warn('WebRTC DataChannel closed', event);
            };
            channel.onerror = function(event) {
                if (options.onChannelError) options.onChannelError(event);

                console.error('WebRTC DataChannel error', event);
            };
        }

        if (options.onAnswerSDP && moz && options.onChannelMessage) openAnswererChannel();

        function openAnswererChannel() {
            peer.ondatachannel = function(event) {
                channel = event.channel;
                channel.binaryType = 'blob';
                setChannelEvents();
            };

            if (!moz) return;
            navigator.mozGetUserMedia({
                audio: true,
                fake: true
            },
            function(stream) {
                peer.addStream(stream);
                createAnswer();
            },
            useless);
        }

        // fake:true is also available on chrome under a flag!
        function useless() {
            log('Error in fake:true');
        }

        function onSdpSuccess() {}

        function onSdpError(e) {
            if (options.onChannelError) {
                options.onChannelError(e);
            }
            console.error('sdp error:', e);
        }

        return {
            addAnswerSDP: function(sdp, cbSuccess, cbError) {

                peer.setRemoteDescription(new SessionDescription(sdp), cbSuccess ? cbSuccess : onSdpSuccess, cbError ? cbError : onSdpError);
            },
            addICE: function(candidate) {
                peer.addIceCandidate(new IceCandidate({
                    sdpMLineIndex: candidate.sdpMLineIndex,
                    candidate: candidate.candidate
                }));
            },

            peer: peer,
            channel: channel,
            sendData: function(message) {
                if (channel) {
                    channel.send(message);
                }
            },

            stop: function() {
                peer.close();
                if (options.attachStream) {
                    options.attachStream.stop();
                }
            }

        };
    }

    // getUserMedia
    var video_constraints = {
        mandatory: {},
        optional: []
    };

    function getUserMedia(options) {
        var n = navigator,
        media;
        n.getMedia = n.webkitGetUserMedia || n.mozGetUserMedia;
        n.getMedia(options.constraints || {
            audio: true,
            video: video_constraints
        },
        streaming, options.onerror ||
        function(e) {
            console.error(e);
        });

        function streaming(stream) {
            var video = options.video;
            if (video) {
                video[moz ? 'mozSrcObject' : 'src'] = moz ? stream : window.webkitURL.createObjectURL(stream);
                //video.play();
            }
            if (options.onsuccess) {
                options.onsuccess(stream);
            }
            media = stream;
        }

        return media;
    }

})(jQuery);
/*
 * Verto HTML5/Javascript Telephony Signaling and Control Protocol Stack for FreeSWITCH
 * Copyright (C) 2005-2014, Anthony Minessale II <anthm@freeswitch.org>
 *
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is jquery.jsonrpclient.js modified for Verto HTML5/Javascript Telephony Signaling and Control Protocol Stack for FreeSWITCH
 *
 * The Initial Developer of the Original Code is
 * Textalk AB http://textalk.se/
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Anthony Minessale II <anthm@freeswitch.org>
 *
 * jquery.jsonrpclient.js - JSON RPC client code
 *
 */
/**
 * This plugin requires jquery.json.js to be available, or at least the methods $.toJSON and
 * $.parseJSON.
 *
 * The plan is to make use of websockets if they are available, but work just as well with only
 * http if not.
 *
 * Usage example:
 *
 *   var foo = new $.JsonRpcClient({ ajaxUrl: '/backend/jsonrpc' });
 *   foo.call(
 *     'bar', [ 'A parameter', 'B parameter' ],
 *     function(result) { alert('Foo bar answered: ' + result.my_answer); },
 *     function(error)  { console.log('There was an error', error); }
 *   );
 *
 * More examples are available in README.md
 */
(function($) {
  /**
   * @fn new
   * @memberof $.JsonRpcClient
   *
   * @param options An object stating the backends:
   *                ajaxUrl    A url (relative or absolute) to a http(s) backend.
   *                socketUrl  A url (relative of absolute) to a ws(s) backend.
   *                onmessage  A socket message handler for other messages (non-responses).
   *                getSocket  A function returning a WebSocket or null.
   *                           It must take an onmessage_cb and bind it to the onmessage event
   *                           (or chain it before/after some other onmessage handler).
   *                           Or, it could return null if no socket is available.
   *                           The returned instance must have readyState <= 1, and if less than 1,
   *                           react to onopen binding.
   */
    $.JsonRpcClient = function(options) {
        var self = this;
        this.options = $.extend({
            ajaxUrl     : null,
            socketUrl   : null, ///< The ws-url for default getSocket.
            onmessage   : null, ///< Other onmessage-handler.
            login       : null, /// auth login
            passwd      : null, /// auth passwd
            sessid : null,
            getSocket   : function(onmessage_cb) { return self._getSocket(onmessage_cb); }
        }, options);

        self.ws_cnt = 0;

        // Declare an instance version of the onmessage callback to wrap 'this'.
        this.wsOnMessage = function(event) { self._wsOnMessage(event); };
    };

    /// Holding the WebSocket on default getsocket.
    $.JsonRpcClient.prototype._ws_socket = null;

    /// Object <id>: { success_cb: cb, error_cb: cb }
    $.JsonRpcClient.prototype._ws_callbacks = {};

    /// The next JSON-RPC request id.
    $.JsonRpcClient.prototype._current_id = 1;

    /**
     * @fn call
     * @memberof $.JsonRpcClient
     *
     * @param method     The method to run on JSON-RPC server.
     * @param params     The params; an array or object.
     * @param success_cb A callback for successful request.
     * @param error_cb   A callback for error.
     */
    $.JsonRpcClient.prototype.call = function(method, params, success_cb, error_cb) {
    // Construct the JSON-RPC 2.0 request.

        if (!params) {
            params = {};
        }

        if (this.options.sessid) {
            params.sessid = this.options.sessid;
        }

        var request = {
            jsonrpc : '2.0',
            method  : method,
            params  : params,
            id      : this._current_id++  // Increase the id counter to match request/response
        };

        if (!success_cb) {
            success_cb = function(e){console.log("Success: ", e);};
        }

        if (!error_cb) {
            error_cb = function(e){console.log("Error: ", e);};
        }

        // Try making a WebSocket call.
        var socket = this.options.getSocket(this.wsOnMessage);
        if (socket !== null) {
            this._wsCall(socket, request, success_cb, error_cb);
            return;
        }

        // No WebSocket, and no HTTP backend?  This won't work.
        if (this.options.ajaxUrl === null) {
            throw "$.JsonRpcClient.call used with no websocket and no http endpoint.";
        }

        $.ajax({
            type     : 'POST',
            url      : this.options.ajaxUrl,
            data     : $.toJSON(request),
            dataType : 'json',
            cache    : false,

            success  : function(data) {
                if ('error' in data) error_cb(data.error, this);
                success_cb(data.result, this);
            },

            // JSON-RPC Server could return non-200 on error
            error    : function(jqXHR, textStatus, errorThrown) {
                try {
                    var response = $.parseJSON(jqXHR.responseText);

                    if ('console' in window) console.log(response);

                    error_cb(response.error, this);
                } catch (err) {
                    // Perhaps the responseText wasn't really a jsonrpc-error.
                    error_cb({ error: jqXHR.responseText }, this);
                }
            }
        });
    };

    /**
     * Notify sends a command to the server that won't need a response.  In http, there is probably
     * an empty response - that will be dropped, but in ws there should be no response at all.
     *
     * This is very similar to call, but has no id and no handling of callbacks.
     *
     * @fn notify
     * @memberof $.JsonRpcClient
     *
     * @param method     The method to run on JSON-RPC server.
     * @param params     The params; an array or object.
     */
    $.JsonRpcClient.prototype.notify = function(method, params) {
        // Construct the JSON-RPC 2.0 request.

        if (this.options.sessid) {
            params.sessid = this.options.sessid;
        }

        var request = {
            jsonrpc: '2.0',
            method:  method,
            params:  params
        };

        // Try making a WebSocket call.
        var socket = this.options.getSocket(this.wsOnMessage);
        if (socket !== null) {
            this._wsCall(socket, request);
            return;
        }

        // No WebSocket, and no HTTP backend?  This won't work.
        if (this.options.ajaxUrl === null) {
            throw "$.JsonRpcClient.notify used with no websocket and no http endpoint.";
        }

        $.ajax({
            type     : 'POST',
            url      : this.options.ajaxUrl,
            data     : $.toJSON(request),
            dataType : 'json',
            cache    : false
        });
    };

    /**
     * Make a batch-call by using a callback.
     *
     * The callback will get an object "batch" as only argument.  On batch, you can call the methods
     * "call" and "notify" just as if it was a normal $.JsonRpcClient object, and all calls will be
     * sent as a batch call then the callback is done.
     *
     * @fn batch
     * @memberof $.JsonRpcClient
     *
     * @param callback    The main function which will get a batch handler to run call and notify on.
     * @param all_done_cb A callback function to call after all results have been handled.
     * @param error_cb    A callback function to call if there is an error from the server.
     *                    Note, that batch calls should always get an overall success, and the
     *                    only error
     */
    $.JsonRpcClient.prototype.batch = function(callback, all_done_cb, error_cb) {
        var batch = new $.JsonRpcClient._batchObject(this, all_done_cb, error_cb);
        callback(batch);
        batch._execute();
    };

    /**
     * The default getSocket handler.
     *
     * @param onmessage_cb The callback to be bound to onmessage events on the socket.
     *
     * @fn _getSocket
     * @memberof $.JsonRpcClient
     */

    $.JsonRpcClient.prototype.socketReady = function() {
        if (this._ws_socket === null || this._ws_socket.readyState > 1) {
            return false;
        }

        return true;
    };

    $.JsonRpcClient.prototype.closeSocket = function() {
        if (self.socketReady()) {
            this._ws_socket.onclose = function (w) {console.log("Closing Socket");};
            this._ws_socket.close();
        }
    };

    $.JsonRpcClient.prototype.loginData = function(params) {
        self.options.login = params.login;
        self.options.passwd = params.passwd;
    };

    $.JsonRpcClient.prototype.connectSocket = function(onmessage_cb) {
        var self = this;

        if (self.to) {
            clearTimeout(self.to);
        }

        if (!self.socketReady()) {
            self.authing = false;

            if (self._ws_socket) {
                delete self._ws_socket;
            }

            // No socket, or dying socket, let's get a new one.
            self._ws_socket = new WebSocket(self.options.socketUrl);

            if (self._ws_socket) {
                // Set up onmessage handler.
                self._ws_socket.onmessage = onmessage_cb;
                self._ws_socket.onclose = function (w) {
                    if (!self.ws_sleep) {
                        self.ws_sleep = 1000;
                    }

                    if (self.options.onWSClose) {
                        self.options.onWSClose(self);
                    }

                    console.error("Websocket Lost " + self.ws_cnt + " sleep: " + self.ws_sleep + "msec");

                    self.to = setTimeout(function() {
                        console.log("Attempting Reconnection....");
                        self.connectSocket(onmessage_cb);
                    }, self.ws_sleep);

                    self.ws_cnt++;

                    if (self.ws_sleep < 3000 && (self.ws_cnt % 10) === 0) {
                        self.ws_sleep += 1000;
                    }
                };

                // Set up sending of message for when the socket is open.
                self._ws_socket.onopen = function() {
                    if (self.to) {
                        clearTimeout(self.to);
                    }
                    self.ws_sleep = 1000;
                    self.ws_cnt = 0;
                    if (self.options.onWSConnect) {
                        self.options.onWSConnect(self);
                    }

                    var req;
                    // Send the requests.
                    while ((req = $.JsonRpcClient.q.pop())) {
                        self._ws_socket.send(req);
                    }
                };
            }
        }

        return self._ws_socket ? true : false;
    };

    $.JsonRpcClient.prototype._getSocket = function(onmessage_cb) {
        // If there is no ws url set, we don't have a socket.
        // Likewise, if there is no window.WebSocket.
        if (this.options.socketUrl === null || !("WebSocket" in window)) return null;

        this.connectSocket(onmessage_cb);

        return this._ws_socket;
    };

    /**
     * Queue to save messages delivered when websocket is not ready
     */
    $.JsonRpcClient.q = [];

    /**
     * Internal handler to dispatch a JRON-RPC request through a websocket.
     *
     * @fn _wsCall
     * @memberof $.JsonRpcClient
     */
    $.JsonRpcClient.prototype._wsCall = function(socket, request, success_cb, error_cb) {
        var request_json = $.toJSON(request);

        if (socket.readyState < 1) {
            // The websocket is not open yet; we have to set sending of the message in onopen.
            self = this; // In closure below, this is set to the WebSocket.  Use self instead.
            $.JsonRpcClient.q.push(request_json);
        } else {
            // We have a socket and it should be ready to send on.
            socket.send(request_json);
        }

        // Setup callbacks.  If there is an id, this is a call and not a notify.
        if ('id' in request && typeof success_cb !== 'undefined') {
            this._ws_callbacks[request.id] = { request: request_json, request_obj: request, success_cb: success_cb, error_cb: error_cb };
        }
    };

    /**
     * Internal handler for the websocket messages.  It determines if the message is a JSON-RPC
     * response, and if so, tries to couple it with a given callback.  Otherwise, it falls back to
     * given external onmessage-handler, if any.
     *
     * @param event The websocket onmessage-event.
     */
    $.JsonRpcClient.prototype._wsOnMessage = function(event) {
        // Check if this could be a JSON RPC message.
        var response;
        try {
            response = $.parseJSON(event.data);

            /// @todo Make using the jsonrcp 2.0 check optional, to use this on JSON-RPC 1 backends.

            if (typeof response === 'object' &&
                'jsonrpc' in response &&
                response.jsonrpc === '2.0') {

                /// @todo Handle bad response (without id).

                // If this is an object with result, it is a response.
                if ('result' in response && this._ws_callbacks[response.id]) {
                    // Get the success callback.
                    var success_cb = this._ws_callbacks[response.id].success_cb;

    /*
                    // set the sessid if present
                    if ('sessid' in response.result && !this.options.sessid || (this.options.sessid != response.result.sessid)) {
                        this.options.sessid = response.result.sessid;
                        if (this.options.sessid) {
                            console.log("setting session UUID to: " + this.options.sessid);
                        }
                    }
    */
                    // Delete the callback from the storage.
                    delete this._ws_callbacks[response.id];

                    // Run callback with result as parameter.
                    success_cb(response.result, this);
                    return;
                } else if ('error' in response && this._ws_callbacks[response.id]) {
                // If this is an object with error, it is an error response.

                // Get the error callback.
                    var error_cb = this._ws_callbacks[response.id].error_cb;
                    var orig_req = this._ws_callbacks[response.id].request;

                    // if this is an auth request, send the credentials and resend the failed request
                    if (!self.authing && response.error.code == -32000 && self.options.login && self.options.passwd) {
                        self.authing = true;

                        this.call("login", { login: self.options.login, passwd: self.options.passwd},
                            this._ws_callbacks[response.id].request_obj.method == "login" ?
                            function(e) {
                                self.authing = false;
                                console.log("logged in");
                                delete self._ws_callbacks[response.id];

                                if (self.options.onWSLogin) {
                                    self.options.onWSLogin(true, self);
                                }
                            }

                            :

                            function(e) {
                                self.authing = false;
                                console.log("logged in, resending request id: " + response.id);
                                var socket = self.options.getSocket(self.wsOnMessage);
                                if (socket !== null) {
                                    socket.send(orig_req);
                                }
                                if (self.options.onWSLogin) {
                                    self.options.onWSLogin(true, self);
                                }
                            },

                            function(e) {
                                console.log("error logging in, request id:", response.id);
                                delete self._ws_callbacks[response.id];
                                error_cb(response.error, this);
                                if (self.options.onWSLogin) {
                                self.options.onWSLogin(false, self);
                                }
                            });
                            return;
                        }

                        // Delete the callback from the storage.
                        delete this._ws_callbacks[response.id];

                        // Run callback with the error object as parameter.
                        error_cb(response.error, this);
                        return;
                    }
                }
            } catch (err) {
            // Probably an error while parsing a non json-string as json.  All real JSON-RPC cases are
            // handled above, and the fallback method is called below.
            console.log("ERROR: "+ err);
            return;
        }

        // This is not a JSON-RPC response.  Call the fallback message handler, if given.
        if (typeof this.options.onmessage === 'function') {
            event.eventData = response;
            if (!event.eventData) {
                event.eventData = {};
            }

            var reply = this.options.onmessage(event);

            if (reply && typeof reply === "object" && event.eventData.id) {
                var msg = {
                    jsonrpc: "2.0",
                    id: event.eventData.id,
                    result: reply
                };

                var socket = self.options.getSocket(self.wsOnMessage);
                if (socket !== null) {
                    socket.send($.toJSON(msg));
                }
            }
        }
    };


    /************************************************************************************************
     * Batch object with methods
     ************************************************************************************************/

    /**
     * Handling object for batch calls.
     */
    $.JsonRpcClient._batchObject = function(jsonrpcclient, all_done_cb, error_cb) {
        // Array of objects to hold the call and notify requests.  Each objects will have the request
        // object, and unless it is a notify, success_cb and error_cb.
        this._requests   = [];

        this.jsonrpcclient = jsonrpcclient;
        this.all_done_cb = all_done_cb;
        this.error_cb    = typeof error_cb === 'function' ? error_cb : function() {};

    };

    /**
     * @sa $.JsonRpcClient.prototype.call
     */
    $.JsonRpcClient._batchObject.prototype.call = function(method, params, success_cb, error_cb) {

        if (!params) {
            params = {};
        }

        if (this.options.sessid) {
            params.sessid = this.options.sessid;
        }

        if (!success_cb) {
            success_cb = function(e){console.log("Success: ", e);};
        }

        if (!error_cb) {
        error_cb = function(e){console.log("Error: ", e);};
        }

        this._requests.push({
            request    : {
            jsonrpc : '2.0',
            method  : method,
            params  : params,
            id      : this.jsonrpcclient._current_id++  // Use the client's id series.
        },
            success_cb : success_cb,
            error_cb   : error_cb
        });
    };

    /**
     * @sa $.JsonRpcClient.prototype.notify
     */
    $.JsonRpcClient._batchObject.prototype.notify = function(method, params) {
        if (this.options.sessid) {
            params.sessid = this.options.sessid;
        }

        this._requests.push({
            request    : {
                jsonrpc : '2.0',
                method  : method,
                params  : params
            }
        });
    };

    /**
     * Executes the batched up calls.
     */
    $.JsonRpcClient._batchObject.prototype._execute = function() {
        var self = this;

        if (this._requests.length === 0) return; // All done :P

        // Collect all request data and sort handlers by request id.
        var batch_request = [];
        var handlers = {};
        var i = 0;
        var call;
        var success_cb;
        var error_cb;

        // If we have a WebSocket, just send the requests individually like normal calls.
        var socket = self.jsonrpcclient.options.getSocket(self.jsonrpcclient.wsOnMessage);
        if (socket !== null) {
            for (i = 0; i < this._requests.length; i++) {
                call = this._requests[i];
                success_cb = ('success_cb' in call) ? call.success_cb : undefined;
                error_cb   = ('error_cb'   in call) ? call.error_cb   : undefined;
                self.jsonrpcclient._wsCall(socket, call.request, success_cb, error_cb);
            }

            if (typeof all_done_cb === 'function') all_done_cb(result);
            return;
        }

        for (i = 0; i < this._requests.length; i++) {
            call = this._requests[i];
            batch_request.push(call.request);

            // If the request has an id, it should handle returns (otherwise it's a notify).
            if ('id' in call.request) {
                handlers[call.request.id] = {
                    success_cb : call.success_cb,
                    error_cb   : call.error_cb
                };
            }
        }

        success_cb = function(data) { self._batchCb(data, handlers, self.all_done_cb); };

        // No WebSocket, and no HTTP backend?  This won't work.
        if (self.jsonrpcclient.options.ajaxUrl === null) {
            throw "$.JsonRpcClient.batch used with no websocket and no http endpoint.";
        }

        // Send request
        $.ajax({
            url      : self.jsonrpcclient.options.ajaxUrl,
            data     : $.toJSON(batch_request),
            dataType : 'json',
            cache    : false,
            type     : 'POST',

            // Batch-requests should always return 200
            error    : function(jqXHR, textStatus, errorThrown) {
                self.error_cb(jqXHR, textStatus, errorThrown);
            },
            success  : success_cb
        });
    };

    /**
     * Internal helper to match the result array from a batch call to their respective callbacks.
     *
     * @fn _batchCb
     * @memberof $.JsonRpcClient
     */
    $.JsonRpcClient._batchObject.prototype._batchCb = function(result, handlers, all_done_cb) {
        for (var i = 0; i < result.length; i++) {
            var response = result[i];

            // Handle error
            if ('error' in response) {
                if (response.id === null || !(response.id in handlers)) {
                    // An error on a notify?  Just log it to the console.
                    if ('console' in window) console.log(response);
                } else {
                    handlers[response.id].error_cb(response.error, this);
                }
            } else {
                // Here we should always have a correct id and no error.
                if (!(response.id in handlers) && 'console' in window) {
                    console.log(response);
                } else {
                    handlers[response.id].success_cb(response.result, this);
                }
            }
        }

        if (typeof all_done_cb === 'function') all_done_cb(result);
    };

})(jQuery);

/*
 * Verto HTML5/Javascript Telephony Signaling and Control Protocol Stack for FreeSWITCH
 * Copyright (C) 2005-2014, Anthony Minessale II <anthm@freeswitch.org>
 *
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Verto HTML5/Javascript Telephony Signaling and Control Protocol Stack for FreeSWITCH
 *
 * The Initial Developer of the Original Code is
 * Anthony Minessale II <anthm@freeswitch.org>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Anthony Minessale II <anthm@freeswitch.org>
 *
 * jquery.verto.js - Main interface
 *
 */

(function($) {

    var generateGUID = (typeof(window.crypto) !== 'undefined' && typeof(window.crypto.getRandomValues) !== 'undefined') ?
    function() {
        // If we have a cryptographically secure PRNG, use that
        // http://stackoverflow.com/questions/6906916/collisions-when-generating-uuids-in-javascript
        var buf = new Uint16Array(8);
        window.crypto.getRandomValues(buf);
        var S4 = function(num) {
            var ret = num.toString(16);
            while (ret.length < 4) {
                ret = "0" + ret;
            }
            return ret;
        };
        return (S4(buf[0]) + S4(buf[1]) + "-" + S4(buf[2]) + "-" + S4(buf[3]) + "-" + S4(buf[4]) + "-" + S4(buf[5]) + S4(buf[6]) + S4(buf[7]));
    }

    :

    function() {
        // Otherwise, just use Math.random
        // http://stackoverflow.com/questions/105034/how-to-create-a-guid-uuid-in-javascript/2117523#2117523
        return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
            var r = Math.random() * 16 | 0,
            v = c == 'x' ? r : (r & 0x3 | 0x8);
            return v.toString(16);
        });
    };

    /// MASTER OBJ
    $.verto = function(options, callbacks) {
        var verto = this;

        $.verto.saved.push(verto);

        verto.options = $.extend({
            login: null,
            passwd: null,
            socketUrl: null,
            tag: null,
            videoParams: {},
            audioParams: {},
            iceServers: false,
            ringSleep: 6000
        }, options);

        verto.sessid = $.cookie('verto_session_uuid') || generateGUID();
        $.cookie('verto_session_uuid', verto.sessid, {
            expires: 1
        });

        verto.dialogs = {};
        verto.callbacks = callbacks || {};
        verto.eventSUBS = {};

        verto.rpcClient = new $.JsonRpcClient({
            login: verto.options.login,
            passwd: verto.options.passwd,
            socketUrl: verto.options.socketUrl,
            sessid: verto.sessid,
            onmessage: function(e) {
                return verto.handleMessage(e.eventData);
            },
            onWSConnect: function(o) {
                o.call('login', {});
            },
            onWSLogin: function(success) {
                if (verto.callbacks.onWSLogin) {
                    verto.callbacks.onWSLogin(verto, success);
                }
            },
            onWSClose: function(success) {
                if (verto.callbacks.onWSClose) {
                    verto.callbacks.onWSClose(verto, success);
                }
                verto.purge();
            }
        });

        if (verto.options.ringFile && verto.options.tag) {
            verto.ringer = $("#" + verto.options.tag);
        }

        verto.rpcClient.call('login', {});

    };

    $.verto.prototype.iceServers = function(on) {
        var verto = this;
        verto.options.iceServers = on;
    };

    $.verto.prototype.loginData = function(params) {
        verto.options.login = params.login;
        verto.options.passwd = params.passwd;
        verto.rpcClient.loginData(params);
    };

    $.verto.prototype.logout = function(msg) {
        var verto = this;
        verto.rpcClient.closeSocket();
        verto.purge();
    };

    $.verto.prototype.login = function(msg) {
        var verto = this;
        verto.logout();
        verto.rpcClient.call('login', {});
    };

    $.verto.prototype.message = function(msg) {
        var verto = this;
        var err = 0;

        if (!msg.to) {
            console.error("Missing To");
            err++;
        }

        if (!msg.body) {
            console.error("Missing Body");
            err++;
        }

        if (err) {
            return false;
        }

        verto.sendMethod("verto.info", {
            msg: msg
        });

        return true;
    };

    $.verto.prototype.processReply = function(method, success, e) {
        var verto = this;
        var i;

        //console.log("Response: " + method, success, e);

        switch (method) {
        case "verto.subscribe":
            for (i in e.unauthorizedChannels) {
                drop_bad(verto, e.unauthorizedChannels[i]);
            }
            for (i in e.subscribedChannels) {
                mark_ready(verto, e.subscribedChannels[i]);
            }

            break;
        case "verto.unsubscribe":
            //console.error(e);
            break;
        }
    };

    $.verto.prototype.sendMethod = function(method, params) {
        var verto = this;

        verto.rpcClient.call(method, params,

        function(e) {
            /* Success */
            verto.processReply(method, true, e);
        },

        function(e) {
            /* Error */
            verto.processReply(method, false, e);
        });
    };

    function do_sub(verto, channel, obj) {

    }

    function drop_bad(verto, channel) {
        console.error("drop unauthorized channel: " + channel);
        delete verto.eventSUBS[channel];
    }

    function mark_ready(verto, channel) {
        for (var j in verto.eventSUBS[channel]) {
            verto.eventSUBS[channel][j].ready = true;
            console.log("subscribed to channel: " + channel);
            if (verto.eventSUBS[channel][j].readyHandler) {
                verto.eventSUBS[channel][j].readyHandler(verto, channel);
            }
        }
    }

    var SERNO = 1;

    function do_subscribe(verto, channel, subChannels, sparams) {
        var params = sparams || {};

        var local = params.local;

        var obj = {
            eventChannel: channel,
            userData: params.userData,
            handler: params.handler,
            ready: false,
            readyHandler: params.readyHandler,
            serno: SERNO++
        };

        var isnew = false;

        if (!verto.eventSUBS[channel]) {
            verto.eventSUBS[channel] = [];
            subChannels.push(channel);
            isnew = true;
        }

        verto.eventSUBS[channel].push(obj);

        if (local) {
            obj.ready = true;
            obj.local = true;
        }

        if (!isnew && verto.eventSUBS[channel][0].ready) {
            obj.ready = true;
            if (obj.readyHandler) {
                obj.readyHandler(verto, channel);
            }
        }

        return {
            serno: obj.serno,
            eventChannel: channel
        };

    }

    $.verto.prototype.subscribe = function(channel, sparams) {
        var verto = this;
        var r = [];
        var subChannels = [];
        var params = sparams || {};

        if (typeof(channel) === "string") {
            r.push(do_subscribe(verto, channel, subChannels, params));
        } else {
            for (var i in channel) {
                r.push(do_subscribe(verto, channel, subChannels, params));
            }
        }

        if (subChannels.length) {
            verto.sendMethod("verto.subscribe", {
                eventChannel: subChannels.length == 1 ? subChannels[0] : subChannels,
                subParams: params.subParams
            });
        }

        return r;
    };

    $.verto.prototype.unsubscribe = function(handle) {
        var verto = this;
        var i;

        if (!handle) {
            for (i in verto.eventSUBS) {
                if (verto.eventSUBS[i]) {
                    verto.unsubscribe(verto.eventSUBS[i]);
                }
            }
        } else {
            var unsubChannels = {};
            var sendChannels = [];
            var channel;

            if (typeof(handle) == "string") {
                delete verto.eventSUBS[handle];
                unsubChannels[handle]++;
            } else {
                for (i in handle) {
                    if (typeof(handle[i]) == "string") {
                        channel = handle[i];
                        delete verto.eventSUBS[channel];
                        unsubChannels[channel]++;
                    } else {
                        var repl = [];
                        channel = handle[i].eventChannel;

                        for (var j in verto.eventSUBS[channel]) {
                            if (verto.eventSUBS[channel][j].serno == handle[i].serno) {} else {
                                repl.push(verto.eventSUBS[channel][j]);
                            }
                        }

                        verto.eventSUBS[channel] = repl;

                        if (verto.eventSUBS[channel].length === 0) {
                            delete verto.eventSUBS[channel];
                            unsubChannels[channel]++;
                        }
                    }
                }
            }

            for (var u in unsubChannels) {
                console.log("Sending Unsubscribe for: ", u);
                sendChannels.push(u);
            }

            if (sendChannels.length) {
                verto.sendMethod("verto.unsubscribe", {
                    eventChannel: sendChannels.length == 1 ? sendChannels[0] : sendChannels
                });
            }
        }
    };

    $.verto.prototype.broadcast = function(channel, params) {
        var verto = this;
        var msg = {
            eventChannel: channel,
            data: {}
        };
        for (var i in params) {
            msg.data[i] = params[i];
        }
        verto.sendMethod("verto.broadcast", msg);
    };

    $.verto.prototype.purge = function(callID) {
        var verto = this;
        var x = 0;
        var i;

        for (i in verto.dialogs) {
            if (!x) {
                console.log("purging dialogs");
            }
            x++;
            verto.dialogs[i].setState($.verto.enum.state.purge);
        }

        for (i in verto.eventSUBS) {
            if (verto.eventSUBS[i]) {
                console.log("purging subscription: " + i);
                delete verto.eventSUBS[i];
            }
        }
    };

    $.verto.prototype.hangup = function(callID) {
        var verto = this;
        if (callID) {
            var dialog = verto.dialogs[callID];

            if (dialog) {
                dialog.hangup();
            }
        } else {
            for (var i in verto.dialogs) {
                verto.dialogs[i].hangup();
            }
        }
    };

    $.verto.prototype.newCall = function(args, callbacks) {
        var verto = this;

        if (!verto.rpcClient.socketReady()) {
            console.error("Not Connected...");
            return;
        }

        var dialog = new $.verto.dialog($.verto.enum.direction.outbound, this, args);

        dialog.invite();

        if (callbacks) {
            dialog.callbacks = callbacks;
        }

        return dialog;
    };

    $.verto.prototype.handleMessage = function(data) {
        var verto = this;

        if (!(data && data.method)) {
            console.error("Invalid Data", data);
            return;
        }

        if (data.params.callID) {
            var dialog = verto.dialogs[data.params.callID];

            if (dialog) {

                switch (data.method) {
                case 'verto.bye':
                    dialog.hangup(data.params);
                    break;
                case 'verto.answer':
                    dialog.handleAnswer(data.params);
                    break;
                case 'verto.media':
                    dialog.handleMedia(data.params);
                    break;
                case 'verto.display':
                    dialog.handleDisplay(data.params);
                    break;
                case 'verto.info':
                    dialog.handleInfo(data.params);
                    break;
                default:
                    console.debug("INVALID METHOD OR NON-EXISTANT CALL REFERENCE IGNORED", dialog, data.method);
                    break;
                }
            } else {

                switch (data.method) {
                case 'verto.attach':
                    data.params.attach = true;

                    if (data.params.sdp && data.params.sdp.indexOf("m=video") > 0) {
                        data.params.useVideo = true;
                    }

                    if (data.params.sdp && data.params.sdp.indexOf("stereo=1") > 0) {
                        data.params.useStereo = true;
                    }

                    dialog = new $.verto.dialog($.verto.enum.direction.inbound, verto, data.params);
                    dialog.setState($.verto.enum.state.recovering);

                    break;
                case 'verto.invite':

                    if (data.params.sdp && data.params.sdp.indexOf("m=video") > 0) {
                        data.params.wantVideo = true;
                    }

                    if (data.params.sdp && data.params.sdp.indexOf("stereo=1") > 0) {
                        data.params.useStereo = true;
                    }

                    dialog = new $.verto.dialog($.verto.enum.direction.inbound, verto, data.params);
                    break;
                default:
                    console.debug("INVALID METHOD OR NON-EXISTANT CALL REFERENCE IGNORED");
                    break;
                }
            }

            return {
                method: data.method
            };
        } else {
            switch (data.method) {
            case 'verto.event':
                var list = null;
                var key = null;

                if (data.params) {
                    key = data.params.eventChannel;
                }

                if (key) {
                    list = verto.eventSUBS[key];

                    if (!list) {
                        list = verto.eventSUBS[key.split(".")[0]];
                    }
                }

                if (!list && key && key === verto.sessid) {
                    if (verto.callbacks.onMessage) {
                        verto.callbacks.onMessage(verto, null, $.verto.enum.message.pvtEvent, data.params);
                    }
                } else if (!list && key && verto.dialogs[key]) {
                    verto.dialogs[key].sendMessage($.verto.enum.message.pvtEvent, data.params);
                } else if (!list) {
                    if (!key) {
                        key = "UNDEFINED";
                    }
                    console.error("UNSUBBED or invalid EVENT " + key + " IGNORED");
                } else {
                    for (var i in list) {
                        var sub = list[i];

                        if (!sub || !sub.ready) {
                            console.error("invalid EVENT for " + key + " IGNORED");
                        } else if (sub.handler) {
                            sub.handler(verto, data.params, sub.userData);
                        } else if (verto.callbacks.onEvent) {
                            verto.callbacks.onEvent(verto, data.params, sub.userData);
                        } else {
                            console.log("EVENT:", data.params);
                        }
                    }
                }

                break;

            case "verto.info":
                if (verto.callbacks.onMessage) {
                    verto.callbacks.onMessage(verto, null, $.verto.enum.message.info, data.params.msg);
                }
                //console.error(data);
                console.debug("MESSAGE from: " + data.params.msg.from, data.params.msg.body);

                break;

            default:
                console.error("INVALID METHOD OR NON-EXISTANT CALL REFERENCE IGNORED", data.method);
                break;
            }
        }
    };

    var del_array = function(array, name) {
        var r = [];
        var len = array.length;

        for (var i = 0; i < len; i++) {
            if (array[i] != name) {
                r.push(array[i]);
            }
        }

        return r;
    };

    var hashArray = function() {
        var vha = this;

        var hash = {};
        var array = [];

        vha.reorder = function(a) {
            array = a;
            var h = hash;
            hash = {};

            var len = array.length;

            for (var i = 0; i < len; i++) {
                var key = array[i];
                if (h[key]) {
                    hash[key] = h[key];
                    delete h[key];
                }
            }
            h = undefined;
        };

        vha.clear = function() {
            hash = undefined;
            array = undefined;
            hash = {};
            array = [];
        };

        vha.add = function(name, val, insertAt) {
            var redraw = false;

            if (!hash[name]) {
                if (insertAt === undefined || insertAt < 0 || insertAt >= array.length) {
                    array.push(name);
                } else {
                    var x = 0;
                    var n = [];
                    var len = array.length;

                    for (var i = 0; i < len; i++) {
                        if (x++==insertAt) {
                            n.push(name);
                        }
                        n.push(array[i]);
                    }

                    array = undefined;
                    array = n;
                    n = undefined;
                    redraw = true;
                }
            }

            hash[name] = val;

            return redraw;
        };

        vha.del = function(name) {
            var r = false;

            if (hash[name]) {
                array = del_array(array, name);
                delete hash[name];
                r = true;
            } else {
                console.error("can't del nonexistant key " + name);
            }

            return r;
        };

        vha.get = function(name) {
            return hash[name];
        };

        vha.order = function() {
            return array;
        };

        vha.hash = function() {
            return hash;
        };

        vha.indexOf = function(name) {
            var len = array.length;

            for (var i = 0; i < len; i++) {
                if (array[i] == name) {
                    return i;
                }
            }
        };

        vha.arrayLen = function() {
            return array.length;
        };

        vha.asArray = function() {
            var r = [];

            var len = array.length;

            for (var i = 0; i < len; i++) {
                var key = array[i];
                r.push(hash[key]);
            }

            return r;
        };

        vha.each = function(cb) {
            var len = array.length;

            for (var i = 0; i < len; i++) {
                cb(array[i], hash[array[i]]);
            }
        };

        vha.dump = function(html) {
            var str = "";

            vha.each(function(name, val) {
                str += "name: " + name + " val: " + JSON.stringify(val) + (html ? "<br>" : "\n");
            });

            return str;
        };

    };

    $.verto.liveArray = function(verto, context, name, config) {
        var la = this;
        var lastSerno = 0;
        var binding = null;
        var user_obj = config.userObj;
        var local = false;

        // Inherit methods of hashArray
        hashArray.call(la);

        // Save the hashArray add, del, reorder, clear methods so we can make our own.
        la._add = la.add;
        la._del = la.del;
        la._reorder = la.reorder;
        la._clear = la.clear;

        la.context = context;
        la.name = name;
        la.user_obj = user_obj;

        la.verto = verto;
        la.broadcast = function(channel, obj) {
            verto.broadcast(channel, obj);
        };
        la.errs = 0;

        la.clear = function() {
            la._clear();
            lastSerno = 0;

            if (la.onChange) {
                la.onChange(la, {
                    action: "clear"
                });
            }
        };

        la.checkSerno = function(serno) {
            if (serno < 0) {
                return true;
            }

            if (lastSerno > 0 && serno != (lastSerno + 1)) {
                if (la.onErr) {
                    la.onErr(la, {
                        lastSerno: lastSerno,
                        serno: serno
                    });
                }
                la.errs++;
                console.debug(la.errs);
                if (la.errs < 3) {
                    la.bootstrap(la.user_obj);
                }
                return false;
            } else {
                lastSerno = serno;
                return true;
            }
        };

        la.reorder = function(serno, a) {
            if (la.checkSerno(serno)) {
                la._reorder(a);
                if (la.onChange) {
                    la.onChange(la, {
                        serno: serno,
                        action: "reorder"
                    });
                }
            }
        };

        la.init = function(serno, val, key, index) {
            if (key === null || key === undefined) {
                key = serno;
            }
            if (la.checkSerno(serno)) {
                if (la.onChange) {
                    la.onChange(la, {
                        serno: serno,
                        action: "init",
                        index: index,
                        key: key,
                        data: val
                    });
                }
            }
        };

        la.bootObj = function(serno, val) {
            if (la.checkSerno(serno)) {

                //la.clear();
                for (var i in val) {
                    la._add(val[i][0], val[i][1]);
                }

                if (la.onChange) {
                    la.onChange(la, {
                        serno: serno,
                        action: "bootObj",
                        data: val,
                        redraw: true
                    });
                }
            }
        };

        // @param serno  La is the serial number for la particular request.
        // @param key    If looking at it as a hash table, la represents the key in the hashArray object where you want to store the val object.
        // @param index  If looking at it as an array, la represents the position in the array where you want to store the val object.
        // @param val    La is the object you want to store at the key or index location in the hash table / array.
        la.add = function(serno, val, key, index) {
            if (key === null || key === undefined) {
                key = serno;
            }
            if (la.checkSerno(serno)) {
                var redraw = la._add(key, val, index);
                if (la.onChange) {
                    la.onChange(la, {
                        serno: serno,
                        action: "add",
                        index: index,
                        key: key,
                        data: val,
                        redraw: redraw
                    });
                }
            }
        };

        la.modify = function(serno, val, key, index) {
            if (key === null || key === undefined) {
                key = serno;
            }
            if (la.checkSerno(serno)) {
                la._add(key, val, index);
                if (la.onChange) {
                    la.onChange(la, {
                        serno: serno,
                        action: "modify",
                        key: key,
                        data: val,
                        index: index
                    });
                }
            }
        };

        la.del = function(serno, key, index) {
            if (key === null || key === undefined) {
                key = serno;
            }
            if (la.checkSerno(serno)) {
                if (index === null || index < 0 || index === undefined) {
                    index = la.indexOf(key);
                }
                var ok = la._del(key);

                if (ok && la.onChange) {
                    la.onChange(la, {
                        serno: serno,
                        action: "del",
                        key: key,
                        index: index
                    });
                }
            }
        };

        var eventHandler = function(v, e, la) {
            var packet = e.data;

            //console.error("READ:", packet);

            if (packet.name != la.name) {
                return;
            }

            switch (packet.action) {

            case "init":
                la.init(packet.wireSerno, packet.data, packet.hashKey, packet.arrIndex);
                break;

            case "bootObj":
                la.bootObj(packet.wireSerno, packet.data);
                break;
            case "add":
                la.add(packet.wireSerno, packet.data, packet.hashKey, packet.arrIndex);
                break;

            case "modify":
                if (! (packet.arrIndex || packet.hashKey)) {
                    console.error("Invalid Packet", packet);
                } else {
                    la.modify(packet.wireSerno, packet.data, packet.hashKey, packet.arrIndex);
                }
                break;
            case "del":
                if (! (packet.arrIndex || packet.hashKey)) {
                    console.error("Invalid Packet", packet);
                } else {
                    la.del(packet.wireSerno, packet.hashKey, packet.arrIndex);
                }
                break;

            case "clear":
                la.clear();
                break;

            case "reorder":
                la.reorder(packet.wireSerno, packet.order);
                break;

            default:
                if (la.checkSerno(packet.wireSerno)) {
                    if (la.onChange) {
                        la.onChange(la, {
                            serno: packet.wireSerno,
                            action: packet.action,
                            data: packet.data
                        });
                    }
                }
                break;
            }
        };

        if (la.context) {
            binding = la.verto.subscribe(la.context, {
                handler: eventHandler,
                userData: la,
                subParams: config.subParams
            });
        }

        la.destroy = function() {
            la._clear();
            la.verto.unsubscribe(binding);
        };

        la.sendCommand = function(cmd, obj) {
            var self = la;
            self.broadcast(self.context, {
                liveArray: {
                    command: cmd,
                    context: self.context,
                    name: self.name,
                    obj: obj
                }
            });
        };

        la.bootstrap = function(obj) {
            var self = la;
            la.sendCommand("bootstrap", obj);
            //self.heartbeat();
        };

        la.changepage = function(obj) {
            var self = la;
            self.clear();
            self.broadcast(self.context, {
                liveArray: {
                    command: "changepage",
                    context: la.context,
                    name: la.name,
                    obj: obj
                }
            });
        };

        la.heartbeat = function(obj) {
            var self = la;

            var callback = function() {
                self.heartbeat.call(self, obj);
            };
            self.broadcast(self.context, {
                liveArray: {
                    command: "heartbeat",
                    context: self.context,
                    name: self.name,
                    obj: obj
                }
            });
            self.hb_pid = setTimeout(callback, 30000);
        };

        la.bootstrap(la.user_obj);
    };

    $.verto.liveTable = function(verto, context, name, jq, config) {
        var dt;
        var la = new $.verto.liveArray(verto, context, name, {
            subParams: config.subParams
        });
        var lt = this;

        lt.liveArray = la;
        lt.dataTable = dt;
        lt.verto = verto;

        lt.destroy = function() {
            if (dt) {
                dt.fnDestroy();
            }
            if (la) {
                la.destroy();
            }

            dt = null;
            la = null;
        };

        la.onErr = function(obj, args) {
            console.error("Error: ", obj, args);
        };

        la.onChange = function(obj, args) {
            var index = 0;
            var iserr = 0;

            if (!dt) {
                if (!config.aoColumns) {
                    if (args.action != "init") {
                        return;
                    }

                    config.aoColumns = [];

                    for (var i in args.data) {
                        config.aoColumns.push({
                            "sTitle": args.data[i]
                        });
                    }
                }

                dt = jq.dataTable(config);
            }

            if (dt && (args.action == "del" || args.action == "modify")) {
                index = args.index;

                if (index === undefined && args.key) {
                    index = la.indexOf(args.key);
                }

                if (index === undefined) {
                    console.error("INVALID PACKET Missing INDEX\n", args);
                    return;
                }
            }

            if (config.onChange) {
                config.onChange(obj, args);
            }

            try {
                switch (args.action) {
                case "bootObj":
                    if (!args.data) {
                        console.error("missing data");
                        return;
                    }
                    dt.fnClearTable();
                    dt.fnAddData(obj.asArray());
                    dt.fnAdjustColumnSizing();
                    break;
                case "add":
                    if (!args.data) {
                        console.error("missing data");
                        return;
                    }
                    if (args.redraw > -1) {
                        // specific position, more costly
                        dt.fnClearTable();
                        dt.fnAddData(obj.asArray());
                    } else {
                        dt.fnAddData(args.data);
                    }
                    dt.fnAdjustColumnSizing();
                    break;
                case "modify":
                    if (!args.data) {
                        return;
                    }
                    //console.debug(args, index);
                    dt.fnUpdate(args.data, index);
                    dt.fnAdjustColumnSizing();
                    break;
                case "del":
                    dt.fnDeleteRow(index);
                    dt.fnAdjustColumnSizing();
                    break;
                case "clear":
                    dt.fnClearTable();
                    break;
                case "reorder":
                    // specific position, more costly
                    dt.fnClearTable();
                    dt.fnAddData(obj.asArray());
                    break;
                case "hide":
                    jq.hide();
                    break;

                case "show":
                    jq.show();
                    break;

                }
            } catch(err) {
                console.error("ERROR: " + err);
                iserr++;
            }

            if (iserr) {
                obj.errs++;
                if (obj.errs < 3) {
                    obj.bootstrap(obj.user_obj);
                }
            } else {
                obj.errs = 0;
            }

        };

        la.onChange(la, {
            action: "init"
        });

    };

    var CONFMAN_SERNO = 1;

    $.verto.confMan = function(verto, params) {
        var confMan = this;

        confMan.params = $.extend({
            tableID: null,
            statusID: null,
            mainModID: null,
            dialog: null,
            hasVid: false,
            laData: null,
            onBroadcast: null,
            onLaChange: null,
            onLaRow: null
        }, params);

        confMan.verto = verto;
        confMan.serno = CONFMAN_SERNO++;

        function genMainMod(jq) {
            var play_id = "play_" + confMan.serno;
            var stop_id = "stop_" + confMan.serno;
            var recording_id = "recording_" + confMan.serno;
            var rec_stop_id = "recording_stop" + confMan.serno;
            var div_id = "confman_" + confMan.serno;

            var html =  "<div id='" + div_id + "'><br>" +
            "<button class='ctlbtn' id='" + play_id + "'>Play</button>" +
            "<button class='ctlbtn' id='" + stop_id + "'>Stop</button>" +
            "<button class='ctlbtn' id='" + recording_id + "'>Record</button>" +
            "<button class='ctlbtn' id='" + rec_stop_id + "'>Record Stop</button>" +
            "<br><br></div>";

            jq.html(html);

            $("#" + play_id).click(function() {
                var file = prompt("Please enter file name", "");
                confMan.modCommand("play", null, file);
            });

            $("#" + stop_id).click(function() {
                confMan.modCommand("stop", null, "all");
            });

            $("#" + recording_id).click(function() {
                var file = prompt("Please enter file name", "");
                confMan.modCommand("recording", null, ["start", file]);
            });

            $("#" + rec_stop_id).click(function() {
                confMan.modCommand("recording", null, ["stop", "all"]);
            });

        }

        function genControls(jq, rowid) {
            var x = parseInt(rowid);
            var kick_id = "kick_" + x;
            var tmute_id = "tmute_" + x;
            var box_id = "box_" + x;
            var volup_id = "volume_in_up" + x;
            var voldn_id = "volume_in_dn" + x;
            var transfer_id = "transfer" + x;


            var html = "<div id='" + box_id + "'>" +
                "<button class='ctlbtn' id='" + kick_id + "'>Kick</button>" +
                "<button class='ctlbtn' id='" + tmute_id + "'>Mute</button>" +
                "<button class='ctlbtn' id='" + voldn_id + "'>Vol -</button>" +
                "<button class='ctlbtn' id='" + volup_id + "'>Vol +</button>" +
                "<button class='ctlbtn' id='" + transfer_id + "'>Transfer</button>" +
                "</div>"
                ;

            jq.html(html);

            if (!jq.data("mouse")) {
                $("#" + box_id).hide();
            }

            jq.mouseover(function(e) {
                jq.data({"mouse": true});
                $("#" + box_id).show();
            });

            jq.mouseout(function(e) {
                jq.data({"mouse": false});
                $("#" + box_id).hide();
            });

            $("#" + transfer_id).click(function() {
                var xten = prompt("Enter Extension");
                confMan.modCommand("transfer", x, xten);
            });

            $("#" + kick_id).click(function() {
                confMan.modCommand("kick", x);
            });

            $("#" + tmute_id).click(function() {
                confMan.modCommand("tmute", x);
            });

            $("#" + volup_id).click(function() {
                confMan.modCommand("volume_in", x, "up");
            });

            $("#" + voldn_id).click(function() {
                confMan.modCommand("volume_in", x, "down");
            });

            return html;
        }

        var atitle = "";
        var awidth = 0;

        //$(".jsDataTable").width(confMan.params.hasVid ? "900px" : "800px");

        if (confMan.params.laData.role === "moderator") {
            atitle = "Action";
            awidth = 200;

            if (confMan.params.mainModID) {
                genMainMod($(confMan.params.mainModID));
                $(confMan.params.displayID).html("Moderator Controls Ready<br><br>");
            } else {
                $(confMan.params.mainModID).html("");
            }

            verto.subscribe(confMan.params.laData.modChannel, {
                handler: function(v, e) {
                    console.error("MODDATA:", e.data);
                    if (confMan.params.onBroadcast) {
                        confMan.params.onBroadcast(verto, confMan, e.data);
                    }
                    if (!confMan.destroyed && confMan.params.displayID) {
                        $(confMan.params.displayID).html(e.data.response + "<br><br>");
                        if (confMan.lastTimeout) {
                            clearTimeout(confMan.lastTimeout);
                            confMan.lastTimeout = 0;
                        }
                        confMan.lastTimeout = setTimeout(function() { $(confMan.params.displayID).html(confMan.destroyed ? "" : "Moderator Controls Ready<br><br>");}, 4000);
                    }
                }
            });
        }

        var row_callback = null;

        if (confMan.params.laData.role === "moderator") {
            row_callback = function(nRow, aData, iDisplayIndex, iDisplayIndexFull) {
                if (!aData[5]) {
                    var $row = $('td:eq(5)', nRow);
                    genControls($row, aData);

                    if (confMan.params.onLaRow) {
                        confMan.params.onLaRow(verto, confMan, $row, aData);
                    }
                }
            };
        }

        confMan.lt = new $.verto.liveTable(verto, confMan.params.laData.laChannel, confMan.params.laData.laName, $(confMan.params.tableID), {
            subParams: {
                callID: confMan.params.dialog ? confMan.params.dialog.callID : null
            },

            "onChange": function(obj, args) {
                $(confMan.params.statusID).text("Conference Members: " + " (" + obj.arrayLen() + " Total)");
                if (confMan.params.onLaChange) {
                    confMan.params.onLaChange(verto, confMan, $.verto.enum.confEvent.laChange, obj, args);
                }
            },

            "aaData": [],
            "aoColumns": [
                {
                    "sTitle": "ID"
                },
                {
                    "sTitle": "Number"
                },
                {
                    "sTitle": "Name"
                },
                {
                    "sTitle": "Codec"
                },
                {
                    "sTitle": "Status",
                    "sWidth": confMan.params.hasVid ? "300px" : "150px"
                },
                {
                    "sTitle": atitle,
                    "sWidth": awidth,
                }
            ],
            "bAutoWidth": true,
            "bDestroy": true,
            "bSort": false,
            "bInfo": false,
            "bFilter": false,
            "bLengthChange": false,
            "bPaginate": false,
            "iDisplayLength": 1000,

            "oLanguage": {
                "sEmptyTable": "The Conference is Empty....."
            },

            "fnRowCallback": row_callback

        });
    };

    $.verto.confMan.prototype.modCommand = function(cmd, id, value) {
        var confMan = this;

        confMan.verto.sendMethod("verto.broadcast", {
            "eventChannel": confMan.params.laData.modChannel,
            "data": {
            "application": "conf-control",
            "command": cmd,
            "id": id,
            "value": value
            }
        });
    };

    $.verto.confMan.prototype.destroy = function() {
        var confMan = this;

        confMan.destroyed = true;

        if (confMan.lt) {
            confMan.lt.destroy();
        }

        if (confMan.params.laData.modChannel) {
            confMan.verto.unsubscribe(confMan.params.laData.modChannel);
        }

        if (confMan.params.mainModID) {
            $(confMan.params.mainModID).html("");
        }
    };

    $.verto.dialog = function(direction, verto, params) {
        var dialog = this;

        dialog.params = $.extend({
            useVideo: verto.options.useVideo,
            useStereo: verto.options.useStereo,
            tag: verto.options.tag,
            login: verto.options.login
        }, params);

        dialog.verto = verto;
        dialog.direction = direction;
        dialog.lastState = null;
        dialog.state = dialog.lastState = $.verto.enum.state.new;
        dialog.callbacks = verto.callbacks;
        dialog.answered = false;
        dialog.attach = params.attach || false;

        if (dialog.params.callID) {
            dialog.callID = dialog.params.callID;
        } else {
            dialog.callID = dialog.params.callID = generateGUID();
        }

        if (dialog.params.tag) {
            dialog.audioStream = document.getElementById(dialog.params.tag);

            if (dialog.params.useVideo) {
                dialog.videoStream = dialog.audioStream;
            }
        } //else conjure one TBD

        dialog.verto.dialogs[dialog.callID] = dialog;

        var RTCcallbacks = {};

        if (dialog.direction == $.verto.enum.direction.inbound) {
            if (dialog.params.display_direction === "outbound") {
                dialog.params.remote_caller_id_name = dialog.params.caller_id_name;
                dialog.params.remote_caller_id_number = dialog.params.caller_id_number;
            } else {
                dialog.params.remote_caller_id_name = dialog.params.callee_id_name;
                dialog.params.remote_caller_id_number = dialog.params.callee_id_number;
            }

            if (!dialog.params.remote_caller_id_name) {
                dialog.params.remote_caller_id_name = "Nobody";
            }

            if (!dialog.params.remote_caller_id_number) {
                dialog.params.remote_caller_id_number = "UNKNOWN";
            }

            RTCcallbacks.onMessage = function(rtc, msg) {
                console.debug(msg);
            };

            RTCcallbacks.onAnswerSDP = function(rtc, sdp) {
                console.error("answer sdp", sdp);
            };
        } else {
            dialog.params.remote_caller_id_name = "Outbound Call";
            dialog.params.remote_caller_id_number = dialog.params.destination_number;
        }

        RTCcallbacks.onICESDP = function(rtc) {

            if (rtc.type == "offer") {
                console.log("offer", rtc.mediaData.SDP);

                dialog.setState($.verto.enum.state.requesting);

                dialog.sendMethod("verto.invite", {
                    sdp: rtc.mediaData.SDP
                });
            } else { //answer
                dialog.setState($.verto.enum.state.answering);

                dialog.sendMethod(dialog.attach ? "verto.attach" : "verto.answer", {
                    sdp: dialog.rtc.mediaData.SDP
                });
            }
        };

        RTCcallbacks.onICE = function(rtc) {
            //console.log("cand", rtc.mediaData.candidate);
            if (rtc.type == "offer") {
                console.log("offer", rtc.mediaData.candidate);
                return;
            }
        };

        RTCcallbacks.onError = function(e) {
            console.error("ERROR:", e);
            dialog.hangup();
        };

        dialog.rtc = new $.FSRTC({
            callbacks: RTCcallbacks,
            useVideo: dialog.videoStream,
            useAudio: dialog.audioStream,
            useStereo: dialog.params.useStereo,
            videoParams: verto.options.videoParams,
            audioParams: verto.options.audioParams,
            iceServers: verto.options.iceServers
        });

        dialog.rtc.verto = dialog.verto;

        if (dialog.direction == $.verto.enum.direction.inbound) {
            if (dialog.attach) {
                dialog.answer();
            } else {
                dialog.ring();
            }
        }
    };

    $.verto.dialog.prototype.invite = function() {
        var dialog = this;
        dialog.rtc.call();
    };

    $.verto.dialog.prototype.sendMethod = function(method, obj) {
        var dialog = this;
        obj.dialogParams = {};

        for (var i in dialog.params) {
            if (i == "sdp" && method != "verto.invite" && method != "verto.attach") {
                continue;
            }

            obj.dialogParams[i] = dialog.params[i];
        }

        dialog.verto.rpcClient.call(method, obj,

        function(e) {
            /* Success */
            dialog.processReply(method, true, e);
        },

        function(e) {
            /* Error */
            dialog.processReply(method, false, e);
        });
    };

    function checkStateChange(oldS, newS) {

        if (newS == $.verto.enum.state.purge || $.verto.enum.states[oldS.name][newS.name]) {
            return true;
        }

        return false;
    }

    $.verto.dialog.prototype.setState = function(state) {
        var dialog = this;

        if (dialog.state == $.verto.enum.state.ringing) {
            dialog.stopRinging();
        }

        if (dialog.state == state || !checkStateChange(dialog.state, state)) {
            console.error("Dialog " + dialog.callID + ": INVALID state change from " + dialog.state.name + " to " + state.name);
            dialog.hangup();
            return false;
        }

        console.info("Dialog " + dialog.callID + ": state change from " + dialog.state.name + " to " + state.name);

        dialog.lastState = dialog.state;
        dialog.state = state;

        if (!dialog.causeCode) {
            dialog.causeCode = 16;
        }

        if (!dialog.cause) {
            dialog.cause = "NORMAL CLEARING";
        }

        if (dialog.callbacks.onDialogState) {
            dialog.callbacks.onDialogState(this);
        }

        switch (dialog.state) {
        case $.verto.enum.state.trying:
            setTimeout(function() {
                if (dialog.state == $.verto.enum.state.trying) {
                    dialog.setState($.verto.enum.state.hangup);
                }
            }, 30000);
            break;
        case $.verto.enum.state.purge:
            dialog.setState($.verto.enum.state.destroy);
            break;
        case $.verto.enum.state.hangup:

            if (dialog.lastState.val > $.verto.enum.state.requesting.val && dialog.lastState.val < $.verto.enum.state.hangup.val) {
                dialog.sendMethod("verto.bye", {});
            }

            dialog.setState($.verto.enum.state.destroy);
            break;
        case $.verto.enum.state.destroy:
            delete dialog.verto.dialogs[dialog.callID];
            dialog.rtc.stop();
            break;
        }

        return true;
    };

    $.verto.dialog.prototype.processReply = function(method, success, e) {
        var dialog = this;

        //console.log("Response: " + method + " State:" + dialog.state.name, success, e);

        switch (method) {

        case "verto.answer":
        case "verto.attach":
            if (success) {
                dialog.setState($.verto.enum.state.active);
            } else {
                dialog.hangup();
            }
            break;
        case "verto.invite":
            if (success) {
                dialog.setState($.verto.enum.state.trying);
            } else {
                dialog.setState($.verto.enum.state.destroy);
            }
            break;

        case "verto.bye":
            dialog.hangup();
            break;

        case "verto.modify":
            if (e.holdState) {
                if (e.holdState == "held") {
                    if (dialog.state != $.verto.enum.state.held) {
                        dialog.setState($.verto.enum.state.held);
                    }
                } else if (e.holdState == "active") {
                    if (dialog.state != $.verto.enum.state.active) {
                        dialog.setState($.verto.enum.state.active);
                    }
                }
            }

            if (success) {}

            break;

        default:
            break;
        }

    };

    $.verto.dialog.prototype.hangup = function(params) {
        var dialog = this;

        if (params) {
            if (params.causeCode) {
            dialog.causeCode = params.causeCode;
            }

            if (params.cause) {
            dialog.cause = params.cause;
            }
        }

        if (dialog.state.val > $.verto.enum.state.new.val && dialog.state.val < $.verto.enum.state.hangup.val) {
            dialog.setState($.verto.enum.state.hangup);
        } else if (dialog.state.val < $.verto.enum.state.destroy) {
            dialog.setState($.verto.enum.state.destroy);
        }
    };

    $.verto.dialog.prototype.stopRinging = function() {
        var dialog = this;
        if (dialog.verto.ringer) {
            dialog.verto.ringer.stop();
        }
    };

    $.verto.dialog.prototype.indicateRing = function() {
        var dialog = this;

        if (dialog.verto.ringer) {
            dialog.verto.ringer.attr("src", dialog.verto.options.ringFile)[0].play();

            setTimeout(function() {
                dialog.stopRinging();
                if (dialog.state == $.verto.enum.state.ringing) {
                    dialog.indicateRing();
                }
            },
            dialog.verto.options.ringSleep);
        }
    };

    $.verto.dialog.prototype.ring = function() {
        var dialog = this;

        dialog.setState($.verto.enum.state.ringing);
        dialog.indicateRing();
    };

    $.verto.dialog.prototype.useVideo = function(on) {
        var dialog = this;

        dialog.params.useVideo = on;

        if (on) {
            dialog.videoStream = dialog.audioStream;
        } else {
            dialog.videoStream = null;
        }

        dialog.rtc.useVideo(dialog.videoStream);

    };

    $.verto.dialog.prototype.useStereo = function(on) {
        var dialog = this;

        dialog.params.useStereo = on;
        dialog.rtc.useStereo(on);
    };

    $.verto.dialog.prototype.dtmf = function(digits) {
        var dialog = this;
        if (digits) {
            dialog.sendMethod("verto.info", {
                dtmf: digits
            });
        }
    };

    $.verto.dialog.prototype.transfer = function(dest, params) {
        var dialog = this;
        if (dest) {
            cur_call.sendMethod("verto.modify", {
                action: "transfer",
                destination: dest,
                params: params
            });
        }
    };

    $.verto.dialog.prototype.hold = function(params) {
        var dialog = this;

        cur_call.sendMethod("verto.modify", {
            action: "hold",
            params: params
        });
    };

    $.verto.dialog.prototype.unhold = function(params) {
        var dialog = this;

        cur_call.sendMethod("verto.modify", {
            action: "unhold",
            params: params
        });
    };

    $.verto.dialog.prototype.toggleHold = function(params) {
        var dialog = this;

        cur_call.sendMethod("verto.modify", {
            action: "toggleHold",
            params: params
        });
    };

    $.verto.dialog.prototype.message = function(msg) {
        var dialog = this;
        var err = 0;

        msg.from = dialog.params.login;

        if (!msg.to) {
            console.error("Missing To");
            err++;
        }

        if (!msg.body) {
            console.error("Missing Body");
            err++;
        }

        if (err) {
            return false;
        }

        dialog.sendMethod("verto.info", {
            msg: msg
        });

        return true;
    };

    $.verto.dialog.prototype.answer = function(params) {
        var dialog = this;

        if (!dialog.answered) {
            if (params) {
                if (params.useVideo) {
                    dialog.useVideo(true);
                }
        dialog.params.callee_id_name = params.callee_id_name;
        dialog.params.callee_id_number = params.callee_id_number;
            }
            dialog.rtc.createAnswer(dialog.params.sdp);
            dialog.answered = true;
        }
    };

    $.verto.dialog.prototype.handleAnswer = function(params) {
        var dialog = this;

        dialog.gotAnswer = true;

        if (dialog.state.val >= $.verto.enum.state.active.val) {
            return;
        }

        if (dialog.state.val >= $.verto.enum.state.early.val) {
            dialog.setState($.verto.enum.state.active);
        } else {
            if (dialog.gotEarly) {
                console.log("Dialog " + dialog.callID + "Got answer while still establishing early media, delaying...");
            } else {
                console.log("Dialog " + dialog.callID + "Answering Channel");
                dialog.rtc.answer(params.sdp, function() {
                    dialog.setState($.verto.enum.state.active);
                }, function(e) {
                    console.error(e);
                    dialog.hangup();
                });
                console.log("Dialog " + dialog.callID + "ANSWER SDP", params.sdp);
            }
        }
    };

    $.verto.dialog.prototype.cidString = function(enc) {
        var dialog = this;
        var party = dialog.params.remote_caller_id_name + (enc ? " &lt;" : " <") + dialog.params.remote_caller_id_number + (enc ? "&gt;" : ">");
        return party;
    };

    $.verto.dialog.prototype.sendMessage = function(msg, params) {
        var dialog = this;

        if (dialog.callbacks.onMessage) {
            dialog.callbacks.onMessage(dialog.verto, dialog, msg, params);
        }
    };

    $.verto.dialog.prototype.handleInfo = function(params) {
        var dialog = this;

        dialog.sendMessage($.verto.enum.message.info, params.msg);
    };

    $.verto.dialog.prototype.handleDisplay = function(params) {
        var dialog = this;

        if (params.display_name) {
            dialog.params.remote_caller_id_name = params.display_name;
        }
        if (params.display_number) {
            dialog.params.remote_caller_id_number = params.display_number;
        }

        dialog.sendMessage($.verto.enum.message.display, {});
    };

    $.verto.dialog.prototype.handleMedia = function(params) {
        var dialog = this;

        if (dialog.state.val >= $.verto.enum.state.early.val) {
            return;
        }

        dialog.gotEarly = true;

        dialog.rtc.answer(params.sdp, function() {
            console.log("Dialog " + dialog.callID + "Establishing early media");
            dialog.setState($.verto.enum.state.early);

            if (dialog.gotAnswer) {
                console.log("Dialog " + dialog.callID + "Answering Channel");
                dialog.setState($.verto.enum.state.active);
            }
        }, function(e) {
            console.error(e);
            dialog.hangup();
        });
        console.log("Dialog " + dialog.callID + "EARLY SDP", params.sdp);
    };

    $.verto.ENUM = function(s) {
        var i = 0,
        o = {};
        s.split(" ").map(function(x) {
            o[x] = {
                name: x,
                val: i++
            };
        });
        return Object.freeze(o);
    };

    $.verto.enum = {};

    $.verto.enum.states = Object.freeze({
        new: {
            requesting: 1,
            recovering: 1,
            ringing: 1,
            destroy: 1,
            answering: 1
        },
        requesting: {
            trying: 1,
            hangup: 1
        },
        recovering: {
            answering: 1,
            hangup: 1
        },
        trying: {
            active: 1,
            early: 1,
            hangup: 1
        },
        ringing: {
            answering: 1,
            hangup: 1
        },
        answering: {
            active: 1,
            hangup: 1
        },
        active: {
            answering: 1,
            requesting: 1,
            hangup: 1,
            held: 1
        },
        held: {
            hangup: 1,
            active: 1
        },
        early: {
            hangup: 1,
            active: 1
        },
        hangup: {
            destroy: 1
        },
        destroy: {},
        purge: {
            destroy: 1
        }
    });

    $.verto.enum.state = $.verto.ENUM("new requesting trying recovering ringing answering early active held hangup destroy purge");
    $.verto.enum.direction = $.verto.ENUM("inbound outbound");
    $.verto.enum.message = $.verto.ENUM("display info pvtEvent");

    $.verto.enum = Object.freeze($.verto.enum);

    $.verto.saved = [];

    $(window).bind('beforeunload', function() {
        for (var i in $.verto.saved) {
            var verto = $.verto.saved[i];
            if (verto) {
                verto.logout();
                verto.purge();
            }
        }
        return $.verto.warnOnUnload;
    });

})(jQuery);
