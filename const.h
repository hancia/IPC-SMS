const int LOGGING_ATTEMPT_MESSAGE_TYPE = 1001;
const int LOGOUT_REQUEST = 1002;
const int LIST_USER = 1003;
const int SEND_NORMAL_MESSAGE = 1010;
const int SEND_MESSAGE_TO_GROUP = 1011;
const int LIST_GROUPS = 1020;
const int LIST_GROUP_USERS = 1021;
const int JOIN_GROUP = 1022;
const int LEAVE_GROUP = 1023;
const int SERVER_RESPONSE = 2000;
const int MESSAGE_FROM = 2001;
static char *const MESSAGE_RESULT_SUCCESS = "success";
static char *const MESSAGE_RESULT_ERROR = "error";
static char *const RESULT_NO_USER = "no user";
static char *const RESULT_NO_GROUP = "no group";
static char *const RESULT_IN_GROUP = "in group";
static char *const RESULT_NO_IN_GROUP = "no in group";
static char *const LOGGING_RESULT_WRONG_PASSWORD = "wrong password";
static char *const LOGGING_RESULT_USER_LOGGED = "user logged";
static char *const LOGGING_RESULT_USER_BLOCKED = "user blocked";

const int serverPublicQueueKey = 7281542;

#define MESSAGE_SIZE sizeof(struct Message) - sizeof(long)

struct Message {
    long type;
    char sender[10];
    char reciever[10];
    char result[100];
    char message[100];
};
