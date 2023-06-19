#include "QGstWebRTCObject.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <QDebug>
//-------------------------------------------------------------------------------------------------------------------------------------------
QGstWebRTCObject::QGstWebRTCObject(QObject *parent) : QObject(parent) {

    GError* err;
    if (gst_init_check(NULL, NULL, &err)) {
        qDebug() << "This program is linked against" << gst_version_string();
    } else {
        qDebug() << "error:" << err->message;
        g_error_free(err);
    }

    data.pipeline = NULL;
    data.webrtc1 = NULL;

    data.obj = this;

    offer = QString();
    icecandidates.clear();

    check_json();
    check_plugins();

    timer = new QTimer(this);
    timer->setInterval(1000);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
}
//-------------------------------------------------------------------------------------------------------------------------------------------
QGstWebRTCObject::~QGstWebRTCObject() {

    DisposePipeline();
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void QGstWebRTCObject::InstantiatePipeline() {

    if(data.pipeline == NULL) {

        data.obj = this;

        GstStateChangeReturn ret;
        GError *error = NULL;




        QString enc;
        char hostname[1024];
        gethostname(hostname, 1024);
				qDebug() << hostname;
				qDebug() << QByteArray(hostname);
				if (strcmp(hostname, "ketron-var-som-mx6")) {
            enc = "x264enc";
        } else {
            enc = "imxvpuenc_h264";
        }
        puts(hostname);

        QString str = QString::fromUtf8("webrtcbin bundle-policy=max-bundle name=sendrecv videotestsrc is-live=true pattern=ball ! videoconvert ! queue ! ") +
                enc +
                QString::fromUtf8(" ! rtph264pay ! queue ! ") +
                QString::fromUtf8(RTP_CAPS_H264) +
                QString::fromUtf8(" ! sendrecv. ");





        data.pipeline = gst_parse_launch (str.toUtf8().constData(), &error);

        if (error) {
            g_printerr ("Failed to parse launch: %s\n", error->message);
            g_error_free (error);

            if (data.pipeline) {

                gst_object_unref(data.pipeline);

                data.pipeline = NULL;
                data.webrtc1 = NULL;
            }
        }

        data.webrtc1 = gst_bin_get_by_name (GST_BIN (data.pipeline), "sendrecv");
        g_assert_nonnull (data.webrtc1);

        // This is the gstwebrtc entry point where we create the offer and so on. It
        // will be called when the pipeline goes to PLAYING.
        g_signal_connect (data.webrtc1, "on-negotiation-needed", G_CALLBACK (on_negotiation_needed), &data);

        // We need to transmit this ICE candidate to the browser via the websockets
        // signalling server. Incoming ice candidates from the browser need to be
        // added by us too, see on_server_message()
        g_signal_connect (data.webrtc1, "on-ice-candidate", G_CALLBACK (on_ice_candidate), &data);
        g_signal_connect (data.webrtc1, "notify::ice-gathering-state", G_CALLBACK (on_ice_gathering_state_notify), &data);

        gst_element_set_state (data.pipeline, GST_STATE_READY);

        // Lifetime is the same as the pipeline itself
        gst_object_unref (data.webrtc1);

        qDebug() << "data.webrtc1:" << data.webrtc1;
        qDebug() << "&data:" << &data;

        ret = gst_element_set_state (GST_ELEMENT (data.pipeline), GST_STATE_PLAYING);

        qDebug() << "gst_element_set_state" << ret << "(must be 1)";

        if (ret == GST_STATE_CHANGE_FAILURE) {
            if (data.pipeline) {

                gst_object_unref(data.pipeline);

                data.pipeline = NULL;
                data.webrtc1 = NULL;
            }
        }
    }
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void QGstWebRTCObject::DisposePipeline() {

    if(data.pipeline != NULL) {

        GstStop(true);

        gst_object_unref (data.pipeline);

        data.pipeline = NULL;
        data.obj = NULL;
    }
}
//-------------------------------------------------------------------------------------------------------------------------------------------
QString QGstWebRTCObject::GetLocalDescritpion() {

    QString str;

    if(data.pipeline != NULL) {

        GstWebRTCSessionDescription* descr;

        g_object_get(G_OBJECT(data.webrtc1), "local-description", &descr, NULL);

        const gchar* type = gst_webrtc_sdp_type_to_string(descr->type);
        const gchar* sdp = gst_sdp_message_as_text(descr->sdp);

        json_object* msg = json_object_new_object();
        json_object_object_add(msg, "type",json_object_new_string((const char*)type));
        json_object_object_add(msg, "sdp", json_object_new_string((const char*)sdp));

        str = QString::fromUtf8(json_object_get_string(msg));

        json_object_put(msg);

        gst_webrtc_session_description_free(descr);
    }

    return str;
}
//-------------------------------------------------------------------------------------------------------------
QString QGstWebRTCObject::GetRemoteDescritpion() {

    QString str;

    if(data.pipeline != NULL) {

        GstWebRTCSessionDescription* descr;

        g_object_get(G_OBJECT(data.webrtc1), "remote-description", &descr, NULL);

        const gchar* type = gst_webrtc_sdp_type_to_string(descr->type);
        const gchar* sdp = gst_sdp_message_as_text(descr->sdp);

        json_object* msg = json_object_new_object();
        json_object_object_add(msg, "type",json_object_new_string((const char*)type));
        json_object_object_add(msg, "sdp", json_object_new_string((const char*)sdp));

        str = QString::fromUtf8(json_object_get_string(msg));

        json_object_put(msg);

        gst_webrtc_session_description_free(descr);
    }

    return str;
}
//-------------------------------------------------------------------------------------------------------------
void QGstWebRTCObject::SetRemoteDescription(QString descr) {

    if(data.pipeline != NULL) {

        json_object *json_msg = json_tokener_parse(descr.toUtf8().data());

        json_object* json_sdp;
        json_object* json_type;

        if (json_object_object_get_ex(json_msg, "sdp", &json_sdp) and json_object_object_get_ex(json_msg, "type", &json_type)) {

            //qDebug() << "\n\nSDP Answer recieved";

            int ret;
            const gchar *sdp;
            const gchar *type;

            type = json_object_get_string(json_type);

            if (g_str_equal (type, "answer")) {

                sdp = json_object_get_string(json_sdp);

                GstSDPMessage *gst_sdp = NULL;
                GstWebRTCSessionDescription *answer = NULL;

                qDebug() << "Recieved SDP type:" << type;
                //qDebug() << sdp;

                ret = gst_sdp_message_new (&gst_sdp);
                g_assert_cmphex (ret, ==, GST_SDP_OK);

                ret = gst_sdp_message_parse_buffer ((guint8 *) sdp, strlen (sdp), gst_sdp);
                g_assert_cmphex (ret, ==, GST_SDP_OK);

                answer = gst_webrtc_session_description_new (GST_WEBRTC_SDP_TYPE_ANSWER, gst_sdp);
                g_assert_nonnull (answer);

                GstPromise *promise = gst_promise_new ();
                g_signal_emit_by_name (data.webrtc1, "set-remote-description", answer, promise);
                gst_promise_interrupt (promise);
                gst_promise_unref (promise);

                gst_webrtc_session_description_free(answer);
            }
        }
        json_object_put(json_msg);
    }
}
//-------------------------------------------------------------------------------------------------------------
//
// Called the first time the pipeline is set into the GST_STATE_PLAYING state
//
//-------------------------------------------------------------------------------------------------------------
void QGstWebRTCObject::on_negotiation_needed (GstElement * element, gpointer user_data) {

    qDebug() << "\non_negotiation_needed";
    qDebug() << "element:" << element;
    qDebug() << "user_data" << user_data;

    GstPromise *promise;

    qDebug() << "gst_promise_new_with_change_func";
    promise = gst_promise_new_with_change_func (on_offer_created, user_data, NULL);

    qDebug() << "g_signal_emit_by_name";
    g_signal_emit_by_name (element, "create-offer", NULL, promise);
}
//-------------------------------------------------------------------------------------------------------------
//
// Called by the create-offer signal. Conatins the offer created by our pipeline, to be sent to the peer
//
//-------------------------------------------------------------------------------------------------------------
void QGstWebRTCObject::on_offer_created (GstPromise * promise, gpointer user_data) {

    QWebRTCServerData* data_ = (QWebRTCServerData*)user_data;

    qDebug() << "\non_offer_created";
    qDebug() << "user_data" << user_data;
    qDebug() << "data" << data_;

    GstWebRTCSessionDescription *offer = NULL;
    const GstStructure *reply;

    g_assert_cmphex (gst_promise_wait (promise), ==, GST_PROMISE_RESULT_REPLIED);
    reply = gst_promise_get_reply (promise);

    /*
    GType type = reply->type;
    const gchar *name = gst_structure_get_name(reply);
    gint n = gst_structure_n_fields(reply);
    qDebug() << "structure name:" << name;
    qDebug() << "structure type:" << type;
    qDebug() << "structure has:" << n << "fields";
    for (gint i = 0; i < n; i ++) {
        const gchar *field = gst_structure_nth_field_name(reply, (guint)i);
        qDebug() << "structure field:" << field;
    }
    gchar * content = gst_structure_to_string (reply);
    qDebug() << "content:" << content;
    */

    gst_structure_get (reply, "offer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &offer, NULL);
    gst_promise_unref (promise);

    promise = gst_promise_new ();
    g_signal_emit_by_name (data_->webrtc1, "set-local-description", offer, promise);
    gst_promise_interrupt (promise);
    gst_promise_unref (promise);

    //
    // Signal
    //
    if (data_ != NULL and data_->obj != NULL) {
        QString str = gst_webrtc_session_descriptor_to_string(offer);
        QMetaObject::invokeMethod(data_->obj, "AddOffer", Qt::BlockingQueuedConnection, Q_ARG(QString, str));
    }

    gst_webrtc_session_description_free (offer);
}
//-------------------------------------------------------------------------------------------------------------
void QGstWebRTCObject::on_ice_candidate (GstElement * webrtc G_GNUC_UNUSED, guint mlineindex, gchar * candidate, gpointer user_data G_GNUC_UNUSED) {

    //qDebug() << "On ICE:" << candidate;

    QWebRTCServerData* data_ = (QWebRTCServerData*)user_data;

    json_object* ice = json_object_new_object();
    json_object_object_add(ice, "candidate",json_object_new_string(candidate));
    json_object_object_add(ice, "sdpMLineIndex",json_object_new_int(mlineindex));

    QString str = QString::fromUtf8(json_object_get_string(ice));

    //
    // Signal
    //
    if (data_ != NULL and data_->obj != NULL) {
        QMetaObject::invokeMethod(data_->obj, "AddIceCandidate", Qt::BlockingQueuedConnection, Q_ARG(QString, str));
    }

    json_object_put(ice);
    Q_UNUSED(webrtc);
}
//-------------------------------------------------------------------------------------------------------------------------------------------
bool QGstWebRTCObject::check_json() {

    qDebug() << "Checking JSON...";
    qDebug() << "Version:" << json_c_version();
    qDebug() << "Version Number:" << json_c_version_num();

    return true;
}
//-------------------------------------------------------------------------------------------------------------------------------------------
QString QGstWebRTCObject::gst_webrtc_session_descriptor_to_string(GstWebRTCSessionDescription *descr) {

    QString str;

    g_assert_nonnull (descr);
    if (descr != NULL) {

        const gchar* type = gst_webrtc_sdp_type_to_string(descr->type);
        const gchar* sdp = gst_sdp_message_as_text(descr->sdp);

        json_object* msg = json_object_new_object();
        json_object_object_add(msg, "type",json_object_new_string((const char*)type));
        json_object_object_add(msg, "sdp", json_object_new_string((const char*)sdp));

        str = QString::fromUtf8(json_object_get_string(msg));

        json_object_put(msg);
    }
    return str;
}
//-------------------------------------------------------------------------------------------------------------------------------------------
bool QGstWebRTCObject::check_plugins (void) {

    qDebug() << "Checking Gst plugins...";

    guint i;
    bool ret;
    GstPlugin *plugin;
    GstRegistry *registry;
    const gchar *needed[] = { "nice", "webrtc", "dtls", "srtp","rtpmanager", "videotestsrc", NULL};
    registry = gst_registry_get ();
    ret = TRUE;
    for (i = 0; i < g_strv_length ((gchar **) needed); i++) {
        plugin = gst_registry_find_plugin (registry, needed[i]);
        if (!plugin) {
            g_print ("Required gstreamer plugin '%s' not found\n", needed[i]);
            ret = FALSE;
            continue;
        }
        gst_object_unref (plugin);
    }
    if (ret) {
        qDebug() << "All plugins OK";
    }
    return ret;
}
//-------------------------------------------------------------------------------------------------------------
void QGstWebRTCObject::on_ice_gathering_state_notify (GstElement * webrtcbin, GParamSpec * pspec, gpointer user_data) {

    QWebRTCServerData* data_ = (QWebRTCServerData*)user_data;

    GstWebRTCICEGatheringState ice_gather_state;
    const gchar *new_state = "unknown";

    g_object_get (webrtcbin, "ice-gathering-state", &ice_gather_state, NULL);
    switch (ice_gather_state) {
    case GST_WEBRTC_ICE_GATHERING_STATE_NEW:
        new_state = "new";
        break;
    case GST_WEBRTC_ICE_GATHERING_STATE_GATHERING:
        new_state = "gathering";
        break;
    case GST_WEBRTC_ICE_GATHERING_STATE_COMPLETE:
        new_state = "complete";
        break;
    }
    qDebug() << "ICE gathering state changed to" << new_state;

    Q_UNUSED(pspec);
    Q_UNUSED(data_);
}
//-------------------------------------------------------------------------------------------------------------
void  QGstWebRTCObject::change_state_handler (GstElement* pipeline, GstState* stateNewPtr) {

#ifdef DEBUG_UDP_GST_CLIENT_CALLBACK
    qInfo() << __PRETTY_FUNCTION__ << *stateNewPtr << pipeline;
#endif

    GstState stateOld;
    GstState stateNew;

    gst_element_get_state(pipeline, &stateOld, NULL,1*GST_SECOND);
    stateNew = *stateNewPtr;

    delete stateNewPtr;

    if(stateOld != stateNew){
        GstStateChangeReturn ret = gst_element_set_state (pipeline, stateNew);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            qDebug() << "Error: cannot change state!";
        }
    }
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void QGstWebRTCObject::AddOffer(QString offer) {

    if (not offer.isEmpty()) {
        this->offer = offer;
        emit onOffer(offer);
    }
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void QGstWebRTCObject::AddIceCandidate(QString candidate) {

    if (not candidate.isEmpty()) {
        this->icecandidates.append(candidate);
        emit onIceCandidate(candidate);
    }
    if (not offer.isEmpty()) {
        timer->start();
    }
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void QGstWebRTCObject::OnOfferRequest() {

    qDebug() << "On Offer Request";
    InstantiatePipeline();
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void QGstWebRTCObject::OnAnswer(QString answer) {

    SetRemoteDescription(answer);
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void QGstWebRTCObject::onTimeout() {

    QString offer = ComposeOffer();

//    json_object *msg = json_object_new_object();
//    json_object* sdp = json_tokener_parse(offer.toUtf8().data());

//    json_object_object_add(msg, "sdp", sdp);
//    QString str = QString::fromUtf8(json_object_get_string(msg));

//    json_object_put(msg);

    emit onOfferReady(offer);
}
//-------------------------------------------------------------------------------------------------------------------------------------------
//
// Create an offer with all the ICE candidates in it
//
//-------------------------------------------------------------------------------------------------------------------------------------------
QString QGstWebRTCObject::ComposeOffer() {

    if (offer.isEmpty()) {
        return QString();
    }

    QString sdp ;

    json_object *json_msg = json_tokener_parse(offer.toUtf8().data());
    json_object* json_sdp;

    if (json_object_object_get_ex(json_msg, "sdp", &json_sdp)) {

        sdp = QString::fromUtf8(json_object_get_string(json_sdp));

        for (int i = 0; i < icecandidates.size(); i ++) {

            json_object *json_ice = json_tokener_parse(icecandidates.at(i).toUtf8().data());
            json_object* json_candidate;

            if (json_object_object_get_ex(json_ice, "candidate", &json_candidate)) {

                QString candidate = QString::fromUtf8(json_object_get_string(json_candidate));

                candidate = candidate.prepend("a=");
                candidate.append("\r\n");

                sdp.append(candidate);
            }
            json_object_put(json_ice);
        }
    }
    json_object_put(json_msg);
    json_msg = NULL;

    json_msg = json_object_new_object();
    json_object_object_add(json_msg, "type",json_object_new_string("offer"));
    json_object_object_add(json_msg, "sdp", json_object_new_string(sdp.toUtf8().constData()));

    QString str = QString::fromUtf8(json_object_get_string(json_msg));

    json_object_put(json_msg);
    json_msg = NULL;

    return str;
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void QGstWebRTCObject::ChangeStateAsync(GstState stateNew) {

    GstState* stateNewPtr;
    stateNewPtr = new GstState(stateNew);

    if(data.pipeline != NULL) {
        gst_element_call_async(data.pipeline,(GstElementCallAsyncFunc)change_state_handler,(GstState*)stateNewPtr,NULL);
    } else {
        delete stateNewPtr;
    }
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void QGstWebRTCObject::ChangeStateSync(GstState stateNew) {

    GstState stateOld;

    if(data.pipeline != NULL) {
        gst_element_get_state(data.pipeline, &stateOld, NULL,1*GST_SECOND);

        if(stateOld != stateNew){
            GstStateChangeReturn ret = gst_element_set_state (data.pipeline, stateNew);
            if (ret == GST_STATE_CHANGE_FAILURE) {
                qDebug() << "Error: cannot change state!";
            }
        }
    }
}
//-------------------------------------------------------------------------------------------------------------------------------------------
GstState QGstWebRTCObject::GetState() {

    GstState state;

    if(data.pipeline != NULL) {
        gst_element_get_state(data.pipeline, &state, NULL,1*GST_SECOND);
    }
    return state;
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void QGstWebRTCObject::GstPlay(bool sync) {

    if (sync)  {
        ChangeStateSync(GST_STATE_PLAYING);
    } else {
        ChangeStateAsync(GST_STATE_PLAYING);
    }
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void QGstWebRTCObject::GstStop(bool sync) {

    if (sync)  {
        ChangeStateSync(GST_STATE_NULL);
    } else {
        ChangeStateAsync(GST_STATE_NULL);
    }
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void QGstWebRTCObject::GstPause(bool sync) {

    if (sync)  {
        ChangeStateSync(GST_STATE_PAUSED);
    } else {
        ChangeStateAsync(GST_STATE_PAUSED);
    }
}



/*


//-------------------------------------------------------------------------------------------------------------
void QGstWebRTCObject::send_sdp_to_peer (GstWebRTCSessionDescription * offer, gpointer user_data) {

    QWebRTCServerData* data_ = (QWebRTCServerData*)user_data;

    qDebug() << "\n\nSend SDP Offer to peer";

    gchar* text = gst_sdp_message_as_text (offer->sdp);
    //qDebug() << "SDP:\n" << text;

    json_object* json_sdp = json_object_new_object();
    json_object_object_add(json_sdp, "type",json_object_new_string("offer"));
    json_object_object_add(json_sdp, "sdp", json_object_new_string((const char*)text));

    json_object* json_msg = json_object_new_object();
    json_object_object_add(json_msg, "sdp", json_sdp);

    //QString str(json_object_get_string(json_msg));

    //qDebug() << str;

    //qint64 n = -1;
    //QMetaObject::invokeMethod(data_->obj, "SendTextMessage", Qt::BlockingQueuedConnection, Q_RETURN_ARG(qint64, n), Q_ARG(QString, str));
    //qDebug() << "\nsent" << n << "bytes";

    json_object_put(json_msg);
    g_free (text);
}


*/
