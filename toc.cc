#include<stdio.h>
#include<string.h>
#include<omnetpp.h>

using namespace omnetpp;

class Toc: public cSimpleModule
{
private:
    int counter = 0; //Counter to receive the packets and send the acknowledgement when it is equal to the window size
    int seq_count = 0; //Variable to keep track of the latest sequence number received successfully
    int seq_count_copy = 0; //Keep a copy of the latest sequence number received successfully, to check if the packet is dropped later or not
    int window = 0; //Variable to store the window size and send it to toc
    int init_msg_flag = 0; //Variable to check if the message is an initial message from tic
    int lost_pkt_flag = 0; //Variable to check if there is a packet loss
    int multiple_lost_pkt_flag = 1; //Variable to check if there are multiple packet loss then send the seq_no one less than the initial packet loss
    int repeat_seq_flag = 0;
protected:
    void handleMessage(cMessage *msg) override;
    void generateMessage();
    void receivedMessage(cMessage *msg);
    void sendMessage(cMessage *msg);
};

Define_Module(Toc);

void Toc:: handleMessage(cMessage *msg) {

    if (uniform(0, 1) < 0.1) {
        EV << "Losing message\n";
        bubble("message lost");

        lost_pkt_flag = 1;
        generateMessage();
        delete msg;
    } else {
        receivedMessage(msg);
        generateMessage();
    }
}

void Toc:: generateMessage()
{
    char msgname[20];
    cMessage *msg;

    /* Generating Initial message, and sending window size */
    if(!init_msg_flag) {
        strcpy(msgname, "Init_toc_msg");
        init_msg_flag = 1;

        msg = new cMessage(msgname);
        msg->addPar("ack_no");
        msg->addPar("win_size");
        msg->par("ack_no").setLongValue(seq_count);
        msg->par("win_size").setLongValue(window);
        send(msg, "out");
    } else { /* Generating messages with Acknowledgment number*/
            if (counter <= window) {
                EV << "Inside generating ack no counter <= window\n";
                counter++;

                if ((counter < window) && (seq_count_copy == (seq_count + 1))) { /*receiving the packet for first time, in right sequence*/
                    EV << "receiving the packet for first time\n";
                    if(seq_count_copy == 255) {
                        EV << " sequence is greater than 255\n";
                        repeat_seq_flag = 1;
                    }
                    seq_count = seq_count + 1;
                    multiple_lost_pkt_flag = 1;
                }
                else if ((counter == window) && (seq_count_copy == (seq_count + 1))) { /*receiving the last packet according to the window size*/
                        EV << "receiving the last packet\n";

                        counter = 0;
                        seq_count = seq_count + 1;
                        sendMessage(msg);
                        if (seq_count_copy == 255) {
                            EV<< "sequence is greater than 255\n";
                            repeat_seq_flag = 1;
                        }

                    }
                    else if (lost_pkt_flag && multiple_lost_pkt_flag) { /*A packet is skipped/lost */
                        EV << "A packet no=" <<seq_count_copy+1<<"is skipped/lost\n";
                        counter = 0;
                        lost_pkt_flag = 0;
                        multiple_lost_pkt_flag = 0;
                        sendMessage(msg);
                    } else {
                        counter = 0;//if 1st packet is lost and after that packets are comig it will inc coun value
                EV<<"first packetlost";
                    }

                }

            }
}

void Toc:: sendMessage(cMessage *msg) {

    char msgname[20];

    strcpy(msgname, "Message from toc");
    msg = new cMessage(msgname);
    msg->addPar("ack_no");
    msg->par("ack_no").setLongValue(seq_count);
    send(msg, "out");
}
void Toc:: receivedMessage(cMessage *msg)
{
    if(msg->par("seq_no").longValue() == 0) { /* Initial Message received from tic, requesting for window size */
        //Retrieve the message
        //print the logs
        //set the window size
        //Delete the message
        EV << "'Message received from: " << msg->getName() << "\n";
        EV << "'Data received: \n" << msg->par("seq_no").longValue() << "\n";
        window = par("win_size");
        delete msg;
    } else { /* Started receiving the packets */
        //Retrieve the message
        //print the logs
        //store the seq_no
        //delete the message
        EV << "'Message received from: " << msg->getName() << "\n";
        EV << "'sequence received: \n" << msg->par("seq_no").longValue() << "\n";
        seq_count_copy = msg->par("seq_no").longValue();
        EV <<"'repeat_seq_flag: " << repeat_seq_flag << "\n";
        if (seq_count_copy == 1 && repeat_seq_flag) { /* Start receiving the packets from 1 after the packet limit reached to 255 */
            repeat_seq_flag = 0;
            seq_count = 0;
        }
        delete msg;
    }
}
