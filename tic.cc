#include<stdio.h>
#include<string.h>
#include<omnetpp.h>

using namespace omnetpp;

class Tic: public cSimpleModule
{
private:
    simtime_t timeout;
    cMessage *timeoutEvent;
    int counter = 3;
    int init_msg_flag = 0;
    int seq_count = 0;
    int win_size = 0;
public:
    Tic();
    virtual ~Tic();
protected:
    virtual void generateMessage();
    virtual void sendMessage(cMessage *msg);
    virtual void receivedMessage(cMessage *msg);
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};


Define_Module(Tic);

Tic::Tic()
{
    timeoutEvent = nullptr;
}

Tic::~Tic()
{
    cancelAndDelete(timeoutEvent);
}
void Tic:: initialize() {
    timeout = 5.0;
    timeoutEvent = new cMessage("timeoutEvent");
    generateMessage();
    EV << "start timer\n";
    scheduleAt(simTime()+timeout, timeoutEvent);
}

void Tic:: handleMessage(cMessage *msg) {

        if(msg == timeoutEvent) {
         if(counter) {
                counter --;
                EV<< "Timeout expired, re-sending message\n";
                bubble("retransmission");
                 if (seq_count == 0) { /* What if the initial message is dropped, resend the initial message again */
                    init_msg_flag = 0;
                    generateMessage();
                }  else {
                    seq_count = seq_count - win_size;
                    EV<<"seq at tic="<<seq_count<<"\n";
                    generateMessage();
                  }
                scheduleAt(simTime()+timeout, timeoutEvent);
            }  else {
                EV << "No response from toc, Exiting the program\n";
            }
        } else  {
            receivedMessage(msg);
            EV<<"timer cancelled";
            cancelEvent(timeoutEvent);
            counter = 3;
            generateMessage();
            scheduleAt(simTime()+timeout, timeoutEvent);
        }
}

void Tic::generateMessage()
{
    char msgname[20];
    int i = 0;
    if(!init_msg_flag) { /* sending initial message, to get the window size */
        init_msg_flag = 1;
        strcpy(msgname, "init_packet");
        cMessage *msg = new cMessage(msgname);
        msg->addPar("seq_no");
        msg->par("seq_no").setLongValue(seq_count);
        sendMessage(msg);
    }
    else { /* start with packet transmission, window size is set by now */

        strcpy(msgname, "Packet from tic");
        //Create message object and set source and destination field.
        for( ; i < win_size; i++) {

            cMessage *msg[win_size];
            seq_count = seq_count + 1;

            msg[i] = new cMessage(msgname);
            msg[i]->addPar("seq_no");
            msg[i]->par("seq_no").setLongValue(seq_count);

            sendMessage(msg[i]);

            if (seq_count >= 255) {
                seq_count = 0;
            }
        }
   }
    return;
}

//Store the message in buffer
//Send the message to toc
void Tic::sendMessage(cMessage *msg) {
    
        send(msg, "out");
    }


//Retrieve the message
//Print the logs on console
//provide the necessary data to send the new message
void Tic::receivedMessage(cMessage *msg) {

    if(msg->par("ack_no").longValue() == 0) {
        //check the window size variable of the message
        //set the window size
        //Delete the msg
        EV << "Message received from: " << msg->getName() << "\n" <<"ackno = "<<msg->par("ack_no").longValue() << "\n";
        EV << "'window size: " << msg->par("win_size").longValue() << "\n";
        win_size = msg->par("win_size").longValue();
        delete msg;
    } else {
        EV << "'Message received from: " << msg->getName() << "\n" << "ackno ="<<msg->par("ack_no").longValue() << "\n";

        seq_count = msg->par("ack_no").longValue();
        if (seq_count >= 255) {
            seq_count = 0;
        }
        delete msg;
    }
}

