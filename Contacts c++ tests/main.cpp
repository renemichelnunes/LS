#include "Contacts.hpp"

int main(int argc, char** argv){
    Contact_list cl;
    Contact c, *c1;
    c.setName("René");
    c.setID("123456");

    cl.add(c);
    printf("%d\n", cl.find(c));
    ContactMessage cm;
    strcpy(cm.messageID, "000000");
    strcpy(cm.dateTime, "26/08/2024");
    strcpy(cm.message, "isto é um teste");
    c1 = cl.getContactByID("123456");
    c1->addMessage(cm);
    c1 = NULL;
    c1 = cl.getContactByID("123456");
    if(c1 == NULL){
        printf("%s\n", "contato não encontrado");
        exit(11);
    }else
        printf("Contato encontrado - %s\n", (*c1).getName().c_str());
    vector<ContactMessage> * messages = c1->getMessagesList();
    for(int i = 0; i < messages->size(); i++){
        printf("vector messages = %s\n", (*messages)[i].message);
    }
    ContactMessage *cm1 = c1->getMessageByID("000000");
    printf("cm1 = %s\n", cm1->message);

    strcpy(cm1->message, "este teste funcionou");
    printf("cm1 altered = %s\n", cm1->message);

    messages = c1->getMessagesList();
    for(int i = 0; i < messages->size(); i++){
        printf("final = %s\n", (*messages)[i].message);
    }

    strcpy((*messages)[0].message, "asda adad a");
    messages = cl.getContactMessages("123456");
    for(int i = 0; i < messages->size(); i++){
        printf("final 2 = %s\n", (*messages)[i].message);
    }
}
