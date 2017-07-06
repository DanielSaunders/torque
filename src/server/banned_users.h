#ifndef BANNED_USERS_H
#define BANNED_USERS_H

#include <string>
#include <map>
#include <list>

#define MAX_RECENT_REQUEST_TIMES 20
#define MAX_REQUEST_FREQUENCY 10

class banned_users
  {
  public:

  bool is_banned_user(std::string orighost, std::string user);

  private:

  std::map<std::pair<std::string, std::string>, std::list<time_t> > users;

  void record_request_from_user(std::pair<std::string, std::string> &looked_up_user, time_t current_time);

  };

#endif
