#ifndef QGSTWEBRTCOBJECT_H
#define QGSTWEBRTCOBJECT_H
//-------------------------------------------------------------------------------------------------------------------------------------------
#include <QObject>
#include <QTimer>
#include "json-c/json.h"
#include "gst/gst.h"
#include <gst/sdp/sdp.h>
#define GST_USE_UNSTABLE_API
#include <gst/webrtc/webrtc.h>
//-------------------------------------------------------------------------------------------------------------------------------------------
#define RTP_CAPS_OPUS "application/x-rtp,media=audio,encoding-name=OPUS,payload="
#define RTP_CAPS_H264 "application/x-rtp,media=video,encoding-name=H264,payload=96"
#define RTP_CAPS_VP8 "application/x-rtp,media=video,encoding-name=VP8,payload=96"
//-------------------------------------------------------------------------------------------------------------------------------------------
class QGstWebRTCObject;
//-------------------------------------------------------------------------------------------------------------------------------------------
typedef struct {
    GstElement* pipeline;
    GstElement* webrtc1;

    QGstWebRTCObject* obj;

} QWebRTCServerData;
//-------------------------------------------------------------------------------------------------------------------------------------------
class QGstWebRTCObject : public QObject {

    Q_OBJECT

public:
    explicit QGstWebRTCObject(QObject *parent = nullptr);
    ~QGstWebRTCObject();

    //
    // GStreamer
    //
    void InstantiatePipeline();
    void DisposePipeline();

    QString ComposeOffer();

    QString GetLocalDescritpion();

    QString GetRemoteDescritpion();
    void SetRemoteDescription(QString descr);

    void ChangeStateAsync(GstState stateNew);
    void ChangeStateSync(GstState stateNew);

    GstState GetState();

    void GstPlay(bool sync = false);
    void GstStop(bool sync = false);
    void GstPause(bool sync = false);

private:
    QWebRTCServerData data;

    QString offer;
    QStringList icecandidates;

    QTimer* timer;

    static bool check_plugins (void);
    static bool check_json (void);

    static QString gst_webrtc_session_descriptor_to_string(GstWebRTCSessionDescription* descr);

    // GStreamer callbacks
    static void on_negotiation_needed (GstElement * element, gpointer user_data);
    static void on_offer_created (GstPromise * promise, gpointer user_data);
    static void send_sdp_to_peer (GstWebRTCSessionDescription * offer, gpointer user_data);

    static void on_ice_candidate (GstElement * webrtc G_GNUC_UNUSED, guint mlineindex, gchar * candidate, gpointer user_data G_GNUC_UNUSED);
    static void on_ice_gathering_state_notify (GstElement * webrtcbin, GParamSpec * pspec, gpointer user_data);

    static void change_state_handler (GstElement* pipeline, GstState* stateNewPtr);

private slots:
    void AddOffer(QString offer);
    void AddIceCandidate(QString candidate);

    void OnOfferRequest();
    void OnAnswer(QString);

    void onTimeout();

signals:
    void onOffer(QString);
    void onIceCandidate(QString);

    void onOfferReady(QString);




};

#endif // QGSTWEBRTCOBJECT_H
