#include <QtGui>
#include "processimage.h"

#include <QMessageBox>
#include <QHBoxLayout>
#include <QButtonGroup>


#include <cstdio>
#include <cstdlib>

#include <unistd.h>
#include "recmgr.h"

#include "DeepCamAPI.h"
#include "IVideoStream.h"

#define NEED_GPSIMU 1
////////////////////////////////////////////////////////

extern int convert_raw_to_rgb_buffer(unsigned char *raw, unsigned char *rgb, bool isGradientBasedInter, int w, int h);

#define RESYNC_NUM 100

ProcessImage::ProcessImage(QWidget *parent)
    :QWidget(parent)
{
    bRenderPaused = false;
    bEnabled = false;
    saved_frames = 0;
    bGetGPSIMU = false;
    bSetAE = false;
    //file_index = 0;

    lblVideoLeft = new QLabel();
    lblVideoRight = new QLabel();
    lblSavedframe = new QLabel();
    lblFramecount = new QLabel();
    lblFramerate = new QLabel();

    QVBoxLayout *mainLayout = new QVBoxLayout();
    QHBoxLayout *hLayout = new QHBoxLayout();
    QHBoxLayout *hLayout2 = new QHBoxLayout();
    QHBoxLayout *hLayout3 = new QHBoxLayout();
    QHBoxLayout *hLayout4 = new QHBoxLayout();
    hLayout->addWidget(lblVideoLeft);
    hLayout->addWidget(lblVideoRight);

    //layout2
    btnRender = new QPushButton();
    btnRender->setText("Stop Render");
    connect(btnRender, SIGNAL(clicked()), this, SLOT(onBtnRenderClick()));

    btnSaveframe = new QPushButton();
    btnSaveframe->setText("Start Save Frame");
    bSaveframe = false;
    saveframe_index = 0;
    connect(btnSaveframe, SIGNAL(clicked()), this, SLOT(onBtnSaveframeClick()));

    recEdit = new QLineEdit();
    recEdit->setMaximumWidth(80);
    recEdit->setText("0");
    QLabel *lblSavenote = new QLabel();
    lblSavenote->setText("(0 for infinite)");
    lblSavedframe->setText("0");

    QLabel *lblGPSIMU = new QLabel();
    lblGPSIMU->setText("GPSIMU Server IP:");
    ipEdit = new QLineEdit();
    ipEdit->setMaximumWidth(200);
    ipEdit->setText("192.168.43.1");
    checkGetGPSIMU = new QCheckBox("Get GPS&IMU info");
    connect(checkGetGPSIMU, SIGNAL(stateChanged(int)), this, SLOT(onGetGPSIMUCheckChanged(int)));

    checkSetAE = new QCheckBox("Enable AE");
    connect(checkSetAE, SIGNAL(stateChanged(int)), this, SLOT(onAESet(int)));

    QButtonGroup *grpFPS = new QButtonGroup(new QWidget);
    radioBtnFPS30 = new QRadioButton("30");
    radioBtnFPS30->setChecked(true);
    grpFPS->addButton(radioBtnFPS30);
    radioBtnFPS20 = new QRadioButton("20");
    grpFPS->addButton(radioBtnFPS20);
    radioBtnFPS15 = new QRadioButton("15");
    grpFPS->addButton(radioBtnFPS15);
    radioBtnFPS10 = new QRadioButton("10");
    grpFPS->addButton(radioBtnFPS10);
    connect(radioBtnFPS30, SIGNAL(clicked(bool)), this, SLOT(onFramerateSet()));
    connect(radioBtnFPS20, SIGNAL(clicked(bool)), this, SLOT(onFramerateSet()));
    connect(radioBtnFPS15, SIGNAL(clicked(bool)), this, SLOT(onFramerateSet()));
    connect(radioBtnFPS10, SIGNAL(clicked(bool)), this, SLOT(onFramerateSet()));

    hLayout2->addWidget(btnRender);
    hLayout2->addWidget(btnSaveframe);
    hLayout2->addWidget(recEdit);
    hLayout2->addWidget(lblSavenote);
    hLayout2->addWidget(lblSavedframe);
    hLayout2->addWidget(lblFramecount);
    hLayout2->addWidget(lblFramerate);
    hLayout2->addWidget(lblGPSIMU);
    hLayout2->addWidget(ipEdit);
    hLayout2->addWidget(checkGetGPSIMU);

    hLayout2->addWidget(checkSetAE);
    QLabel *lblFPS = new QLabel();
    lblFPS->setText("FPS:");
    hLayout2->addWidget(lblFPS);
    hLayout2->addWidget(radioBtnFPS30);
    hLayout2->addWidget(radioBtnFPS20);
    hLayout2->addWidget(radioBtnFPS15);
    hLayout2->addWidget(radioBtnFPS10);

    hLayout2->addStretch();
    //hlayout3
   QButtonGroup *grp2=new QButtonGroup(new QWidget);
   radioBtn1 = new QRadioButton("left");
   radioBtn1->setChecked(true);
   grp2->addButton(radioBtn1);
   hLayout3->addWidget(radioBtn1);
   radioBtn2 = new QRadioButton("right");
   grp2->addButton(radioBtn2);
   hLayout3->addWidget(radioBtn2);
   radioBtn3 = new QRadioButton("both");
   grp2->addButton(radioBtn3);
   hLayout3->addWidget(radioBtn3);

    QLabel *alable = new QLabel();
    alable->setText("RGB gain(0~7.97):");
    hLayout3->addWidget(alable);

    QDoubleValidator* aDoubleValidator = new QDoubleValidator();
    aDoubleValidator->setRange(0, 7.97);

    //QRegExp rx("^-?(90|[1-8]?\\d(\\.\\d{1,4})?)$");

    QRegExp rx("^([0-7](\\.\\d{1,5}))$");
    QRegExpValidator* aRegExpValidator = new QRegExpValidator();
    aRegExpValidator->setRegExp(rx);

    RgainEdit = new QLineEdit();
    RgainEdit->setMaximumWidth(100);
    RgainEdit->setValidator(aRegExpValidator);
    hLayout3->addWidget(RgainEdit);

    GgainEdit = new QLineEdit();
    GgainEdit->setMaximumWidth(100);
    GgainEdit->setValidator(aRegExpValidator);
    hLayout3->addWidget(GgainEdit);

    BgainEdit = new QLineEdit();
    BgainEdit->setMaximumWidth(100);
    BgainEdit->setValidator(aRegExpValidator);
    hLayout3->addWidget(BgainEdit);

    QPushButton *gainGetBtn = new QPushButton();
    gainGetBtn->setText("get");
    connect(gainGetBtn,SIGNAL(clicked()), this, SLOT(onGainGet()));
    hLayout3->addWidget(gainGetBtn);
    QPushButton *gainSetBtn = new QPushButton();
    gainSetBtn->setText("set");
    connect(gainSetBtn,SIGNAL(clicked()), this, SLOT(onGainSet()));
    hLayout3->addWidget(gainSetBtn);

    QLabel *lableexposure = new QLabel();
    lableexposure->setText("Expo:");
    hLayout3->addWidget(lableexposure);
    exposureEdit = new QLineEdit();
    exposureEdit->setMaximumWidth(100);
    hLayout3->addWidget(exposureEdit);
    QPushButton *exposureGetbtn = new QPushButton();
    exposureGetbtn->setText("get");
    connect(exposureGetbtn,SIGNAL(clicked()), this, SLOT(onExposureGet()));
    hLayout3->addWidget(exposureGetbtn);
    QPushButton *exposureSetbtn = new QPushButton();
    exposureSetbtn->setText("set");
    connect(exposureSetbtn,SIGNAL(clicked()), this, SLOT(onExposureSet()));
    hLayout3->addWidget(exposureSetbtn);

    //hLayout3->addStretch();

    QLabel *lblFixedexpo = new QLabel();
    lblFixedexpo->setText("Fixed Expo:");
    hLayout3->addWidget(lblFixedexpo);

    QButtonGroup *grp1=new QButtonGroup(new QWidget);
    radioBtnExp100 = new QRadioButton("100");
    radioBtnExp100->setChecked(true);
    grp1->addButton(radioBtnExp100);
    hLayout3->addWidget(radioBtnExp100);
    radioBtnExp200 = new QRadioButton("200");
    grp1->addButton(radioBtnExp200);
    hLayout3->addWidget(radioBtnExp200);
    radioBtnExp400 = new QRadioButton("400");
    grp1->addButton(radioBtnExp400);
    hLayout3->addWidget(radioBtnExp400);
    radioBtnExp800 = new QRadioButton("800");
    grp1->addButton(radioBtnExp800);
    hLayout3->addWidget(radioBtnExp800);
    radioBtnExp1200 = new QRadioButton("1200");
    grp1->addButton(radioBtnExp1200);
    hLayout3->addWidget(radioBtnExp1200);
    connect(radioBtnExp100, SIGNAL(clicked(bool)), this, SLOT(radioExpoChange()));
    connect(radioBtnExp200, SIGNAL(clicked(bool)), this, SLOT(radioExpoChange()));
    connect(radioBtnExp400, SIGNAL(clicked(bool)), this, SLOT(radioExpoChange()));
    connect(radioBtnExp800, SIGNAL(clicked(bool)), this, SLOT(radioExpoChange()));
    connect(radioBtnExp1200, SIGNAL(clicked(bool)), this, SLOT(radioExpoChange()));

    hLayout3->addStretch();

    lblGPS = new QLabel();
    lblACC = new QLabel();
    lblGYR = new QLabel();
    lblMAG = new QLabel();
    lblGPS->setText("GPS: 0.000000, 0.000000, 0.000000;    ");
    lblACC->setText("ACC: 0.000000, 0.000000, 0.000000;    ");
    lblGYR->setText("GYR: 0.000000, 0.000000, 0.000000;    ");
    lblMAG->setText("MAG: 0.000000, 0.000000, 0.000000;    ");
    hLayout4->addWidget(lblGPS);
    hLayout4->addWidget(lblACC);
    hLayout4->addWidget(lblGYR);
    hLayout4->addWidget(lblMAG);

    //mainLayout
    mainLayout->addLayout(hLayout2);
    mainLayout->addLayout(hLayout3);
    mainLayout->addLayout(hLayout4);
    mainLayout->addLayout(hLayout);
//    mainLayout->addLayout(hLayout4);
    mainLayout->addStretch();

    setLayout(mainLayout);;
    setWindowTitle(tr("Stereo Vision"));

    qRegisterMetaType<TopGear::IVideoFramePtr>("TopGear::IVideoFramePtr");
    connect(this, SIGNAL(onstereoframe(TopGear::IVideoFramePtr, TopGear::IVideoFramePtr)),
            this, SLOT(showstereoframe(TopGear::IVideoFramePtr, TopGear::IVideoFramePtr)),
            Qt::QueuedConnection );
    connect(this, SIGNAL(ondevexception(int)),
            this, SLOT(handledeviceexception(int)),
            Qt::QueuedConnection );

    auto deepcam = TopGear::DeepCamAPI::Instance();
    camera = deepcam.CreateCamera(TopGear::Camera::CamaroDual);
    if (camera)
    {
        ioControl = std::dynamic_pointer_cast<TopGear::IDeviceControl>(camera);
        cameraControl = std::dynamic_pointer_cast<TopGear::ICameraControl>(camera);
        if (cameraControl)
            cameraControl->Flip(true,false);
        TopGear::IVideoStream::RegisterFrameCallback(*camera,
               &ProcessImage::onGetStereoFrame, this);
        //labeldevinfo->setText(QString("devinfo:%1").arg(item->GetDeviceInfo().c_str()));
        //Get optimized video format
        auto index = camera->GetOptimizedFormatIndex(format);

        prgb1 = std::unique_ptr<uchar[]>(new uchar[format.Height*format.Width*3]);
        prgb2 = std::unique_ptr<uchar[]>(new uchar[format.Height*format.Width*3]);

        //auto formats = camera->GetAllFormats();
        camera->SetCurrentFormat(index);
        camera->StartStream();
    }

    timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(ontimer()));
    timer->start(500);
}


ProcessImage::~ProcessImage()
{
    if (timer)
        timer->stop();
    if (camera)
        camera->StopStream();
}


void ProcessImage::ontimer()
{
    int drops = dropcount.load(std::memory_order_relaxed);
    dropcount = 0;
    lblFramerate->setText(QString("fps:%1  framedrops:%2").arg((int)fc.GetFrameRate()).arg(drops));
}

void ProcessImage::onGetStereoFrame(TopGear::IVideoStream &sender, std::vector<TopGear::IVideoFramePtr> &frames)//this is called in sub-thread
{
    (void)sender;
    if (frames.size()!=2)
        return;
    qDebug("frameidx:  %d",frames[0]->GetFrameIdx());
    static int lastIdx = -1;
    auto currentIdx = frames[0]->GetFrameIdx();

    //lblFramecount->setText(QString("%1").arg(currentIdx));

    if (lastIdx==-1)
    {
        lastIdx=currentIdx;
        dropcount = 0;
    }
    else
    {
        if (++lastIdx >= RESYNC_NUM)
            lastIdx = 0;
        if (currentIdx>lastIdx)
        {
            dropcount += currentIdx-lastIdx;
            lastIdx = currentIdx;
        }
        else if (currentIdx<lastIdx)
        {
            dropcount += currentIdx-lastIdx;
            lastIdx = currentIdx;
        }
        else if (currentIdx<lastIdx)
        {
            dropcount.fetch_add(RESYNC_NUM - lastIdx + currentIdx);
            lastIdx = currentIdx;
        }
    }

    fc.NewFrame();
    emit onstereoframe(frames[0], frames[1]);
}

void ProcessImage::onDeviceException(int err){
    emit ondevexception(err);
}

void ProcessImage::display_error(QString err)
{
    //QMessageBox::warning(this,tr("error"), err,QMessageBox::Yes);
    qDebug() <<err;
}


void ProcessImage::onBtnRenderClick()
{

    bRenderPaused = !bRenderPaused;
    if(bRenderPaused)
        btnRender->setText("Start Render");
    else
        btnRender->setText("Stop Render");

}

void ProcessImage::onBtnSaveframeClick()
{

    bSaveframe = !bSaveframe;
    if(bSaveframe)
    {
        btnSaveframe->setText("Stop Save Frame");
        //stereocam->EnableRecord(recEdit->text().toInt());
        max_save = recEdit->text().toInt();
        saveframe_index = 0;
        saved_frames = 0;
        recEdit->setEnabled(false);
    }
    else
    {
        btnSaveframe->setText("Start Save Frame");
        //stereocam->DisableRecord();
        recEdit->setEnabled(true);
    }

}

void ProcessImage::onGetGPSIMUCheckChanged(int state)
{

//    if (state == Qt::Checked)
//    {
//        gpsimuinfo.init(ipEdit->text().toLatin1().data(), 4321);
//        gpsimuinfo.getinfo_start();
//        bGetGPSIMU = true;
//        ipEdit->setEnabled(false);
//    }
//    else {
//        gpsimuinfo.getinfo_stop();
//        gpsimuinfo.deinit();
//        bGetGPSIMU = false;
//        ipEdit->setEnabled(true);
//    }
}

// add info from shift of (8 x 16bits)
void ProcessImage::addInfoToFrame(void *frame, unsigned char *info, int infolen)
{
//    u_int16_t *p = (uint16_t *)((VideoFrameRef)frame)->pdata;
//    p = p + 8;
//    for (int i = 0; i < infolen; i++)
//    {
//        p[i*2] &= 0x0FFF; p[i*2] |= (info[i] & 0x00F0) << 8;
//        p[i*2+1] &= 0x0FFF; p[i*2+1] |= (info[i] & 0x000F) << 12;
//    }
}

void ProcessImage::getInfoFromFrame(void *frame, unsigned char *info, int infolen)
{
//    u_int16_t *p = (uint16_t *)((VideoFrameRef)frame)->pdata;
//    p = p + 8;
//    for (int i = 0; i < infolen; i++)
//    {
//        info[i] = (p[i*2] >> 8) & 0xF0;
//        info[i] |= ((p[i*2+1] >> 12) & 0x0F);
//    }
}

void ProcessImage::showstereoframe(TopGear::IVideoFramePtr master, TopGear::IVideoFramePtr slave)
{
    uint8_t *raw1,*raw2;
    uint32_t stride;
    master->LockBuffer(&raw1,&stride);
    slave->LockBuffer(&raw2,&stride);


    static int framecount = 0;
    static int ccc = 0;
    //qDebug("stereo %d", ((VideoFrameRef)left)->frameidx);

//    //GPSIMUValue value1, value2;
//    if (bGetGPSIMU)
//    {

//        //gpsimuinfo.GetCurrInfo(&value1);
//        unsigned char data[64];
//        //memcpy(data, &value1.acc[0], 12+12+12+24);
//        memcpy(data, value1.acc, 12);
//        memcpy(data+12, value1.gyr, 12);
//        memcpy(data+24, value1.mag, 12);
//        memcpy(data+36, value1.gps, 24);
//        //addInfoToFrame(left, data, 60);
//    }

    if (bSaveframe)
    {
        if (saveframe_index == 0)
        {
            qDebug("=================Start recroding frames...");
            system("mkdir -p ~/tmp/RAW");
            char sss[256];
            sprintf(sss,"%s/tmp/RAW/all%02d.raw",getenv("HOME"),ccc);
            REC_Open(sss);
            ccc ++;
        }

        if (REC_Push(raw1, raw2, format.Height*stride, saveframe_index) <0)
            qDebug("---RECORD drop %d---", saveframe_index);

        saveframe_index ++;

        if (saveframe_index % 200 == 0)
        {
            qDebug("===record frames %d", saveframe_index);
        }

        saved_frames = saveframe_index;

        if (max_save > 0 && saveframe_index >= max_save)
        {
            qDebug("=================record frames DONE!!!");
            REC_Close();
            saveframe_index = 0;
            onBtnSaveframeClick();
        }
    }
    else if (saveframe_index > 0)
    {
        REC_Close();
        qDebug("=================record frames terminate(%d)!", saveframe_index);
        saveframe_index = 0;
    }

    if (!bRenderPaused)
    {
//        if (bGetGPSIMU)
//        {
//            unsigned char info[64];
//            //getInfoFromFrame((VideoFrameRef)left, info, 60);
//            memcpy(value2.acc, info,    12);
//            memcpy(value2.gyr, info+12, 12);
//            memcpy(value2.mag, info+24, 12);
//            memcpy(value2.gps, info+36, 24);

//        }


//         int sz = PIX_HEIGHT * PIX_WIDTH * 3;
//        rgb1 = new unsigned char[sz];
//        rgb2 = new unsigned char[sz];

        convert_raw_to_rgb_buffer(raw1, prgb1.get(), false , format.Width, format.Height);
        convert_raw_to_rgb_buffer(raw2, prgb2.get(), false , format.Width, format.Height);



//        int lumi_left = 0;
//        int lumi_right = 0;
//        if (!bSetAE)
//        {
//            convert_raw_to_rgb_buffer(((VideoFrameRef)left)->pdata, rgb1, false);
//            convert_raw_to_rgb_buffer(((VideoFrameRef)right)->pdata, rgb2, false);
//        }
//        else
//        {
//            convert_raw_to_rgb_buffer_with_lumi_calc(((VideoFrameRef)left)->pdata, rgb1, &lumi_left, 0, 0, PIX_WIDTH, PIX_HEIGHT);
//            convert_raw_to_rgb_buffer_with_lumi_calc(((VideoFrameRef)right)->pdata, rgb2, &lumi_right, 0, 0, PIX_WIDTH, PIX_HEIGHT);
//        }


        static int count = 0;
        count++;
        if (count == 100000)
             count = 0;
        if (bGetGPSIMU && count % 10 == 0)
        {
#if 0
            lblGPS->setText(QString("GPS: %1, %2, %3;    ").arg(value2.gps[0], 0, '.', 6).arg(value2.gps[1], 0, '.', 6).arg(value2.gps[2], 0, '.', 6));
            lblACC->setText(QString("ACC: %1, %2, %3;    ").arg(value2.acc[0], 0, '.', 6).arg(value2.acc[1], 0, '.', 6).arg(value2.acc[2], 0, '.', 6));
            lblGYR->setText(QString("GYR: %1, %2, %3;    ").arg(value2.gyr[0], 0, '.', 6).arg(value2.gyr[1], 0, '.', 6).arg(value2.gyr[2], 0, '.', 6));
            lblMAG->setText(QString("MAG: %1, %2, %3;    ").arg(value2.mag[0], 0, '.', 6).arg(value2.mag[1], 0, '.', 6).arg(value2.mag[2], 0, '.', 6));

            fprintf(stderr, "<===== %f, %f, %f\r\n", value1.acc[0], value1.acc[1], value1.acc[2]);
            fprintf(stderr, "       %f, %f, %f\r\n", value1.gyr[0], value1.gyr[1], value1.gyr[2]);
            fprintf(stderr, "       %f, %f, %f\r\n", value1.mag[0], value1.acc[1], value1.mag[2]);
            fprintf(stderr, "       %f, %f, %f\r\n", value1.gps[0], value1.gps[1], value1.gps[2]);
            fprintf(stderr, "=====> %f, %f, %f\r\n", value2.acc[0], value2.acc[1], value2.acc[2]);
            fprintf(stderr, "       %f, %f, %f\r\n", value2.gyr[0], value2.gyr[1], value2.gyr[2]);
            fprintf(stderr, "       %f, %f, %f\r\n", value2.mag[0], value2.acc[1], value2.mag[2]);
            fprintf(stderr, "       %f, %f, %f\r\n", value2.gps[0], value2.gps[1], value2.gps[2]);
#endif
        }

        QImage * frame = new QImage(prgb1.get(),format.Width, format.Height,QImage::Format_RGB888);
        lblVideoLeft->setPixmap(QPixmap::fromImage(*frame,Qt::AutoColor).scaled(800,450));
        delete frame;

        frame = new QImage(prgb2.get(),format.Width, format.Height,QImage::Format_RGB888);
        lblVideoRight->setPixmap(QPixmap::fromImage(*frame,Qt::AutoColor).scaled(800,450));
        delete frame;
    }

    master->UnlockBuffer();
    slave->UnlockBuffer();



//        if (bSetAE)
//        {
//            int target_lumi;
//            int tolerance = 1;
//            int cur_lumi = (lumi_left + lumi_right) / 2;
//            float smooth = 0.5; // 0.1~1

//            if (((UVCDevice*)stereocam)->get_devicetype() == AR0134)
//                target_lumi = 128;
//            else
//                target_lumi = 128;

//            if (abs(target_lumi - cur_lumi) > tolerance)
//            {
//                int cur_expo;
//                cur_expo = ((UVCDevice*)stereocam)->get_stereoexposure();
//                if (cur_expo == 0)
//                    cur_expo = 1;

//                int desiredExpo = cur_expo + (cur_expo * (target_lumi - cur_lumi) * smooth + cur_lumi - 1) / cur_lumi;
//                if (desiredExpo > 4000) // set exposure <= 1000 to keep 30fps. if exposure > 1000, then framerate = 30 * 1000 / exposure;
//                        desiredExpo = 4000;
//                ((UVCDevice*)stereocam)->set_stereoexposure(desiredExpo);
//                //fprintf(stderr, "cur_expo: %d; cur_lumi: %d; desiredExpo: %d\n", cur_expo, cur_lumi, desiredExpo);
//            }
//        }
//    }
//    else {
//        stereocam->releaseVideoFrame((VideoFrameRef)left);
//        stereocam->releaseVideoFrame((VideoFrameRef)right);
//    }

    lblSavedframe->setText(QString("%1").arg(saved_frames));
    lblFramecount->setText(QString(" frames:%1").arg(++framecount));
//    fc.NewFrame();
}

void ProcessImage::handledeviceexception(int err)
{
    //stereocam->close();
    if (camera)
        camera->StopStream();
    QMessageBox msgBox;
    msgBox.setText("----Device Lost!!!!!");
    msgBox.exec();
}

//void ProcessImage::showstereoframe(unsigned char *rgb1, unsigned char *rgb2,unsigned int frameid)
//{
//    if (!bRenderPaused)
//    {
//        QImage * frame = new QImage(rgb1,PIX_WIDTH,PIX_HEIGHT,QImage::Format_RGB888);
//        lblVideoLeft->setPixmap(QPixmap::fromImage(*frame,Qt::AutoColor).scaled(640,360));
//        delete frame;

//        frame = new QImage(rgb2,PIX_WIDTH,PIX_HEIGHT,QImage::Format_RGB888);
//        lblVideoRight->setPixmap(QPixmap::fromImage(*frame,Qt::AutoColor).scaled(640,360));
//        delete frame;
//    }
//    /*if (savecheckbox->checkState() == Qt::Checked && file_index < 420)
//    {
//        char filename[256] = {0};

//        sprintf(filename, "/home/xinghua/tmp/RGB/left%04d.rgb", file_index);
//        FILE *fp = fopen(filename, "wb");
//        fwrite( rgb1, PIX_WIDTH*PIX_HEIGHT*3, 1,fp);
//        fclose(fp);
//        //write_png(filename, rgb1, PIX_WIDTH, PIX_HEIGHT, 8, 3);
//        sprintf(filename, "/home/xinghua/tmp/RGB/right%04d.rgb", file_index);
//        fp = fopen(filename, "wb");
//        fwrite( rgb2, PIX_WIDTH*PIX_HEIGHT*3, 1, fp);
//        fclose(fp);
//        //write_png(filename, rgb2, PIX_WIDTH, PIX_HEIGHT, 8, 3);
//        file_index ++;
//    }*/

//    lblFramecount->setText(QString("%1").arg(frameid));
//}

// gain value: 0 ~ 9.7
void ProcessImage::getGain(float *fGainR, float *fGainG, float *fGainB)
{
    if (cameraControl == nullptr)
        return;

    unsigned short gainR=0, gainG=0, gainB=0;
    cameraControl->GetGain(gainR,gainG,gainB);

    *fGainR = ((gainR & 0x00ff) >> 5 ) + (gainR & 0x001f) * 0.03125;
    *fGainG = ((gainG & 0x00ff) >> 5 ) + (gainG & 0x001f) * 0.03125;
    *fGainB = ((gainB & 0x00ff) >> 5 ) + (gainB & 0x001f) * 0.03125;
}

// gain value: 0 ~ 9.7
void ProcessImage::setGain(float fGainR, float fGainG, float fGainB)
{
    if (cameraControl == nullptr)
        return;
    unsigned short gainR, gainG, gainB;

    gainR = (((int)fGainR) << 5);
    int dGainR = (int)((fGainR - (int)fGainR + 0.01563) / 0.03125);
    if (dGainR > 31)
        dGainR = 31;
    gainR += dGainR;

    gainG = (((int)fGainG) << 5);
    int dGainG = (int)((fGainG - (int)fGainG + 0.01563) / 0.03125);
    if (dGainG > 31)
        dGainG = 31;
    gainG += dGainG;

    gainB = (((int)fGainB) << 5);
    int dGainB = (int)((fGainB - (int)fGainB + 0.01563) / 0.03125);
    if (dGainB > 31)
        dGainB = 31;
    gainB += dGainB;

    cameraControl->SetGain(gainR,gainG,gainB);
}

void ProcessImage::onGainGet()
{
    unsigned short gainR, gainG, gainB;
//    if(radioBtn3->isChecked())
//    {
//        //stereocam->get_wb_gain(0, &gainR, &gainG, &gainB);//read left one by default
//        stereocam->get_gain(&gainR, &gainG, &gainB, 0);
//    }
//    else if(radioBtn1->isChecked())
//        //stereocam->get_wb_gain(0, &gainR, &gainG, &gainB);
//        stereocam->get_gain(&gainR, &gainG, &gainB, 0);
//    else  if(radioBtn2->isChecked())
//        //stereocam->get_wb_gain(1, &gainR, &gainG, &gainB);
//        stereocam->get_gain(&gainR, &gainG, &gainB, 1);


    float fGainR, fGainG, fGainB;

    fGainR = ((gainR & 0x00ff) >> 5 ) + (gainR & 0x001f) * 0.03125;
    fGainG = ((gainG & 0x00ff) >> 5 ) + (gainG & 0x001f) * 0.03125;
    fGainB = ((gainB & 0x00ff) >> 5 ) + (gainB & 0x001f) * 0.03125;


    //RgainEdit->setText(QString::number(gainR,10));
    //GgainEdit->setText(QString::number(gainG,10));
    //BgainEdit->setText(QString::number(gainB,10));
    RgainEdit->setText(QString("%1").arg(fGainR));
    GgainEdit->setText(QString("%1").arg(fGainG));
    BgainEdit->setText(QString("%1").arg(fGainB));
}

void ProcessImage::onGainSet()
{
    unsigned short gainR, gainG, gainB;
    float fGainR, fGainG, fGainB;
    bool ok;
    //gainR = RgainEdit->text().toInt(&ok, 10);
    //gainG = GgainEdit->text().toInt(&ok, 10);
    //gainB = BgainEdit->text().toInt(&ok, 10);

    fGainR = RgainEdit->text().toFloat();
    fGainG = GgainEdit->text().toFloat();
    fGainB = BgainEdit->text().toFloat();

    gainR = (((int)fGainR) << 5);
    int dGainR = (int)((fGainR - (int)fGainR + 0.01563) / 0.03125);
    if (dGainR > 31)
        dGainR = 31;
    gainR += dGainR;

    gainG = (((int)fGainG) << 5);
    int dGainG = (int)((fGainG - (int)fGainG + 0.01563) / 0.03125);
    if (dGainG > 31)
        dGainG = 31;
    gainG += dGainG;

    gainB = (((int)fGainB) << 5);
    int dGainB = (int)((fGainB - (int)fGainB + 0.01563) / 0.03125);
    if (dGainB > 31)
        dGainB = 31;
    gainB += dGainB;


//    if (radioBtn1->isChecked() || radioBtn3->isChecked())
//        //stereocam->set_wb_gain(0, gainR, gainG, gainB);
//        stereocam->set_gain(gainR, gainG,gainB, 0);
//    if (radioBtn2->isChecked() || radioBtn3->isChecked())
//        //stereocam->set_wb_gain(1, gainR, gainG, gainB);
//        stereocam->set_gain(gainR, gainG,gainB, 1);
}

void ProcessImage::onExposureGet()
{
    unsigned short shutter = 0;

//    if(radioBtn3->isChecked())
//    {
//        //stereocam->get_exposure(0,&shutter);//read left camera by default
//        stereocam->get_exposure(&shutter,0);
//    }
//    else if(radioBtn1->isChecked())
//        //stereocam->get_exposure(0,&shutter);
//        stereocam->get_exposure(&shutter,0);
//    else if(radioBtn2->isChecked())
//        //stereocam->get_exposure(1,&shutter);
//        stereocam->get_exposure(&shutter,1);

    exposureEdit->setText(QString::number(shutter,10));
}

void ProcessImage::onAESet(int state)
{
    if (state == Qt::Checked)
    {
        /*
        unsigned short regaddr[] = {0x3100, 0x3112, 0x3114, 0x3116, 0x3118, 0x3102, 0x3104, 0x3126, 0x311C, 0x311E};
        unsigned short regval[] =  {0x0001, 0x029F, 0x008C, 0x02C0, 0x005B, 0x0384, 0x1000, 0x0080, 0x03DD, 0x0002};
        stereocam->set_registers(regaddr,regval, 10, 0);
        stereocam->set_registers(regaddr,regval, 10, 1);
        */
        bSetAE = true;
    }
    else {
        /*
        unsigned short regaddr[] = {0x3100};
        unsigned short regval[] = {0x0000};
        stereocam->set_registers(regaddr,regval, 1, 0);
        stereocam->set_registers(regaddr,regval, 1, 1);
        */
        bSetAE = false;
    }
}

void ProcessImage::onFramerateSet()
{
    // 1650 * 1.5 = 30fps
    unsigned short regaddr[] = {0x300C};    // LINE_LENGTH_PCK
    unsigned short regval[] = {2475};

    if (sender() == radioBtnFPS30)
    {
        regval[0] = 2475; //1650 * 1.5;
    }
    else if (sender() == radioBtnFPS20)
    {
        regval[0] = 3713; //2475 * 1.5
    }
    else if (sender() == radioBtnFPS15)
    {
        regval[0] = 4950; //2475 * 2
    }
    else if (sender() == radioBtnFPS10)
    {
        regval[0] = 7425; //2475 * 2
    }

//    stereocam->set_registers(regaddr,regval, 1, 0);
//    stereocam->set_registers(regaddr,regval, 1, 1);
}

void ProcessImage::onExposureSet()
{
    unsigned short shutter;
    bool ok;
    shutter = exposureEdit->text().toInt(&ok, 10);
//    if (radioBtn1->isChecked() || radioBtn3->isChecked())
//        //stereocam->set_exposure(0,shutter);
//        stereocam->set_exposure(shutter,0);
//    if (radioBtn2->isChecked() || radioBtn3->isChecked())
//        //stereocam->set_exposure(1,shutter);
//        stereocam->set_exposure(shutter,1);
}

void ProcessImage::radioExpoChange()
{
    if (sender() == radioBtnExp100)
    {
        cameraControl->SetExposure(100);
    }
    else if (sender() == radioBtnExp200)
    {
        cameraControl->SetExposure(200);
    }
    else if (sender() == radioBtnExp400)
    {
        cameraControl->SetExposure(400);
    }
    else if (sender() == radioBtnExp800)
    {
        cameraControl->SetExposure(800);
    }
    else if (sender() == radioBtnExp1200)
    {
        cameraControl->SetExposure(1200);
    }
}

#if 0
int ProcessImage::convert_yuv_to_rgb_pixel(int y, int u, int v)
{
 unsigned int pixel32 = 0;
 unsigned char *pixel = (unsigned char *)&pixel32;
 int r, g, b;
 r = y + (1.370705 * (v-128));
 g = y - (0.698001 * (v-128)) - (0.337633 * (u-128));
 b = y + (1.732446 * (u-128));
 if(r > 255) r = 255;
 if(g > 255) g = 255;
 if(b > 255) b = 255;
 if(r < 0) r = 0;
 if(g < 0) g = 0;
 if(b < 0) b = 0;
 pixel[0] = r * 220 / 256;
 pixel[1] = g * 220 / 256;
 pixel[2] = b * 220 / 256;
 return pixel32;
}
/*yuv格式转换为rgb格式*/
#endif