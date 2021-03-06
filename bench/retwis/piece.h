#ifndef RETWIS_PIECE_H_
#define RETWIS_PIECE_H_

#include "deptran/piece.h"

namespace rococo {


// ======= Add Users txn =======
#define RETWIS_ADD_USERS              1
#define RETWIS_ADD_USERS_NAME         "ADD_USERS"
#define RETWIS_ADD_USERS_0            10 //R & W VAR1
#define RETWIS_ADD_USERS_1            11 //W VAR2
#define RETWIS_ADD_USERS_2            12 //W VAR3


// ======= Follow txn =======
#define RETWIS_FOLLOW                  2
#define RETWIS_FOLLOW_NAME             "FOLLOW"
#define RETWIS_FOLLOW_0                20 //R & W VAR1
#define RETWIS_FOLLOW_1                21 //R & W VAR2


// ======= Post Tweet txn =======
#define RETWIS_POST_TWEET              3
#define RETWIS_POST_TWEET_NAME         "POST_TWEET"
#define RETWIS_POST_TWEET_0            30 //R & W VAR1
#define RETWIS_POST_TWEET_1            31 //R & W VAR2
#define RETWIS_POST_TWEET_2            32 //R & W VAR3
#define RETWIS_POST_TWEET_3            33 //W VAR4
#define RETWIS_POST_TWEET_4            34 //W VAR5


// ======= Get Timeline txn =======
#define RETWIS_GET_TIMELINE              4
#define RETWIS_GET_TIMELINE_NAME         "GET_TIMELINE"
#define RETWIS_GET_TIMELINE_P(I)            (40+I) //R VAR I



#define RETWIS_VAR_ADD_USERS_1       (1001)
#define RETWIS_VAR_ADD_USERS_2       (1002)
#define RETWIS_VAR_ADD_USERS_3       (1003)


#define RETWIS_VAR_FOLLOW_1    (2001)
#define RETWIS_VAR_FOLLOW_2    (2002)


#define RETWIS_VAR_POST_TWEET_1     (3001)
#define RETWIS_VAR_POST_TWEET_2     (3002)
#define RETWIS_VAR_POST_TWEET_3     (3003)
#define RETWIS_VAR_POST_TWEET_4     (3004)
#define RETWIS_VAR_POST_TWEET_5     (3005)


#define RETWIS_VAR_GET_TIMELINE_CNT    (4001)
#define RETWIS_VAR_GET_TIMELINE(I)     (4002+I)


extern char RETWIS_TB[];
extern char RETWIS_TB[];

class RetwisPiece: public Piece {
 protected:
  // add users
  virtual void RegAddUsers();

  // follow
  virtual void RegFollow();

  // post tweet
  virtual void RegPostTweet();

  // get timeline
  virtual void RegGetTimeline();



 public:
  virtual void reg_all();

  RetwisPiece();

  virtual ~RetwisPiece();

};

}

#endif
