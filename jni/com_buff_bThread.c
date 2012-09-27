#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <jni.h>
#include <android/log.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "ADARNA-SensorAquisitionMod", __VA_ARGS__))
#define SAMPLES 80 //needs to be changed
#define MAX_IN_LEN 80
#define MAX_FLT_LEN 63
#define BUFF_LEN (MAX_FLT_LEN - 1 + MAX_IN_LEN)
#define FLT_ORD 6
float coeffs[(FLT_ORD) +1] = {0.0303,0.1187,0.2193,0.2636,0.2193,0.1187,0.0303};

struct inBuff {
  long 	 timestamp;
  double inX[SAMPLES];
  double inY[SAMPLES];
  double inZ[SAMPLES];
};

// buffer object
struct datBuffer {
       struct inBuff buffDat;
       struct datBuffer *next;

};

//structs for queue
struct buffQueue {
       struct datBuffer *head;
       struct datBuffer *tail;
};

struct datBuffer* pollBuff(struct buffQueue* s);
struct buffQueue *addBuff(struct buffQueue* s, struct datBuffer* p);
struct buffQueue *newQueue(void);
struct buffQueue *freeQueue(struct buffQueue*s);
struct buffQueue *removeElems(struct buffQueue*s);

struct buffQueue *bufferList; //will hold raw data
struct buffQueue *fBufferList; //will hold filtered data
struct datBuffer *dataAcc; //one buffer per sensor
struct buffQueue *kBufferList; //will hold data process by KF

void *filtManager(); //loads data onto the filter
void *streamManager(); //keeps on reading data from files. simulates sensor callbacks.
void *kalmanManager(); //loads prefiltered data into the Kalman filter.

FILE *fInX;
FILE *fInY;
FILE *fInZ;

double insampX[BUFF_LEN];
double insampY[BUFF_LEN];
double insampZ[BUFF_LEN];

//flags
short sensAcqDone = 0;
short preFiltDone = 0;
short kalmanDone  = 0;

short buffAddFlag = 0;
short filtAddFlag = 0;

pthread_t filtManager_t;
pthread_t strmManager_t;

static pthread_mutex_t preFilt_Mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t kalmanDone_Mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t buffMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t kalmanMutex = PTHREAD_MUTEX_INITIALIZER;


JNIEXPORT void JNICALL Java_com_buff_bThread_firBuff
  (JNIEnv *env , jclass class){
    //intitalize stuff

    memset(insampX,0,sizeof(insampX));
    memset(insampY,0,sizeof(insampY));
    memset(insampZ,0,sizeof(insampZ));

    bufferList = newQueue();
    fBufferList = newQueue();
    kBufferList = newQueue();

    if( (fInX = fopen("/sdcard/inputx.txt","r")) != NULL)
    	LOGI("inputx.txt lives!");
    if((fInY = fopen("/sdcard/inputy.txt","r"))!= NULL)
    	LOGI("inputy.txt lives!");
    if((fInZ = fopen("/sdcard/inputz.txt","r"))!= NULL)
    	LOGI("inputz.txt lives!");


    pthread_create(&strmManager_t,NULL,streamManager,NULL);
    pthread_create(&filtManager_t,NULL,filtManager,NULL);

    freeQueue(kBufferList);
    freeQueue(bufferList);
    freeQueue(fBufferList);
    free(fBufferList);
    free(bufferList);
}

void *kalmanManager()
{
	struct datBuffer *tmp;

	while(1)
	{
		if(filtAddFlag)
		{

		}
		else
		{
			usleep(10);
		}
	}
}

void *filtManager()
{
	struct datBuffer *ftmp;
	struct datBuffer *tmp;
	double accX,accY,accZ;
	float *coeffp;
	double *inputXp;
	double *inputYp;
	double *inputZp;
	int n;
	int k;

	//replace with flags in actual implementation
	while(!feof(fInX) && !feof(fInY) && !feof(fInZ))
	{	//filtering routine
		if(buffAddFlag)
		{
			LOGI("HELLO THIS IS NEW");
			pthread_mutex_lock(&buffMutex);
			buffAddFlag = 0;
			pthread_mutex_unlock(&buffMutex);
			//poll queue of read sensor values
			tmp = pollBuff(bufferList);

			memcpy(&insampX[FLT_ORD + 1 ],tmp->buffDat.inX,SAMPLES*sizeof(double));
			memcpy(&insampY[FLT_ORD + 1 ],tmp->buffDat.inY,SAMPLES*sizeof(double));
			memcpy(&insampZ[FLT_ORD + 1 ],tmp->buffDat.inZ,SAMPLES*sizeof(double));
			//retrieve head element in queue

			for(n = 0; n < SAMPLES; n++)
			{
				inputXp = &insampX[FLT_ORD + n];
				inputYp = &insampY[FLT_ORD + n];
				inputZp = &insampZ[FLT_ORD + n];

				accX = accY = accZ = 0;

				for(k = 0; k < FLT_ORD + 1; k++)
				{
					accX += (*inputXp--)*coeffs[k];
					accY += (*inputYp--)*coeffs[k];
					accZ += (*inputZp--)*coeffs[k];

				}

				ftmp->buffDat.inX[n] = accX;
				ftmp->buffDat.inY[n] = accY;
				ftmp->buffDat.inZ[n] = accZ;
			}
			LOGI("ADDING TO FILTERED BUFFER QUEUE");
			addBuff(fBufferList, ftmp);
			//set flag to indicate that prefiltered data is available for the Kalman Filter.
			filtAddFlag = 1;
			//free(ftmp);
			LOGI("SETTING INSAMP BACK TO TIME");

			memmove(&insampX[0],&insampX[SAMPLES], FLT_ORD*sizeof(double));
			LOGI("MEMMOVE1");
			memmove(&insampY[0],&insampY[SAMPLES], FLT_ORD*sizeof(double));
			LOGI("MEMMOVE2");
			memmove(&insampZ[0],&insampZ[SAMPLES], FLT_ORD*sizeof(double));
			LOGI("MEMMOVE3");
			free(tmp);
			LOGI("FREETEMP");
		}
		else
		{
			usleep(10);

		}
	}

}
 void *streamManager()
 {
	    char strBuffX[32];
	    char strBuffY[32];
	    char strBuffZ[32];
	    int x;
	    int buffCount = 0;
	    long time=0;
	    LOGI("Inside streamManager.");
	    //initialize string buffers.
	    LOGI("Initializing string buffers...");
	    memset(strBuffX,0,sizeof(strBuffX));
	    memset(strBuffY,0,sizeof(strBuffY));
	    memset(strBuffZ,0,sizeof(strBuffZ));

	 while(!feof(fInX) && !feof(fInY) && !feof(fInZ))
	 {
		 LOGI("HELLO THERE");
		 //LOGI("%d", dataAcc);
		 dataAcc = malloc(sizeof(struct datBuffer));

		 LOGI("HELLO THERE2");
		 for(x = 0; x < SAMPLES && (!feof(fInX) && !feof(fInY) && !feof(fInZ)); x++)
		 {	//scan and sleep
			 fscanf(fInX,"%s",strBuffX);
			 fscanf(fInY,"%s",strBuffY);
			 fscanf(fInZ,"%s",strBuffZ);
			 dataAcc->buffDat.timestamp = time;
			 dataAcc->buffDat.inX[x] = atof(strBuffX);
			 dataAcc->buffDat.inY[x] = atof(strBuffY);
			 dataAcc->buffDat.inZ[x] = atof(strBuffZ);

			 usleep(1000);
			 	time+= 1000; //simulated timestamp
			    memset(strBuffX,0,sizeof(strBuffX));
			    memset(strBuffY,0,sizeof(strBuffY));
			    memset(strBuffZ,0,sizeof(strBuffZ));
		 }

		 if (x < SAMPLES) //ran out of samples to read
		 {
			 //pad the remaining array entries with (SAMPLE - 1 - x) zeros.
			 for(; x < SAMPLES; x++)
			 {
				 dataAcc->buffDat.inX[x] = 0.0;
				 dataAcc->buffDat.inY[x] = 0.0;
				 dataAcc->buffDat.inZ[x] = 0.0;
			 }
		 }

		 LOGI("Buffer #%d",++buffCount);

		 //push into Queue
		 addBuff(bufferList, dataAcc);
		 LOGI("SETTING MUTEX");
		pthread_mutex_lock(&buffMutex);

		 buffAddFlag = 1;

		pthread_mutex_unlock(&buffMutex);
	 }
	 sensAcqDone = 1;
	 LOGI("Closing files...");
	 fclose(fInX);
	 fclose(fInY);
	 fclose(fInZ);
 }


 struct datBuffer* pollBuff(struct buffQueue* s)
 {
        struct datBuffer *tmp;
        if(s == NULL)
        {
             LOGI("Queue is not initialized.");
             return NULL;
        }
        else if (s->head == NULL)
        {
        	LOGI("Queue is empty.");
             return NULL;
         }
         tmp = s->head;
        if (s->head == s->tail)
        {
             s->tail = NULL;

        }
        s->head = s->head->next;
        tmp->next = NULL;

        return tmp;

 }
 struct buffQueue *newQueue(void){
        struct buffQueue* p = malloc(sizeof(p));

        p->head = p->tail = NULL;

        return p;
 }

 struct buffQueue *addBuff(struct buffQueue *s, struct datBuffer *dat){
         dat->next = NULL;

          if ( s->head == NULL )
         {     s->head = s->tail = dat;
              return s;
         }

              s->tail->next = dat;
              s->tail = dat;
              return s;
 }

 struct buffQueue *removeElems(struct buffQueue* s)
 {
        struct datBuffer* h = NULL;
        struct datBuffer* p = NULL;

        if(s == NULL)
        {
             return s;
         }
             else if (NULL == s->head && NULL == s->tail )
             {
                  return s;
              }
        else if (NULL == s->head || NULL == s->tail )
        {
             return s;
         }

         h = s->head;
         p = h->next;
         free(h);
         s->head = p;
         if( NULL == s->head )  s->tail = s->head;   /* The element tail was pointing to is free(), so we need an update */

         return s;
 }
 struct buffQueue *freeQueue(struct buffQueue* s)
 {
        while(s->head){
           removeElems(s);
                       }
        return s;
 }

