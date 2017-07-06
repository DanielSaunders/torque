#include <time.h>

#include "banned_users.h"

using namespace std;


/*
 * record_request_from_user() - Every time a user makes a request, make a note
 * of that request, so as to make sure a user is not making requests too
 * frequently.
 */

void banned_users::record_request_from_user(

    pair<string, string>                   &looked_up_user,
    time_t                                  current_time)

  {
  map<pair<string, string>, list<time_t> >::iterator user_it;

  // If user already present in db, add current time, otherwise, insert a new
  // user into db
  if ((user_it = this->users.find(looked_up_user)) != this->users.end())
    {
    if (user_it->second.size() > MAX_RECENT_REQUEST_TIMES)
      {
      user_it->second.pop_front();
      }

      user_it->second.push_back(current_time);
    }
  else
    {
    list<time_t> times;
    times.push_back(current_time);

    pair<pair<string, string>, list<time_t> > record_to_insert(looked_up_user, times);

    this->users.insert(record_to_insert);
    }
  }



/*
 * is_banned_user() - Check if the given user/host has been banned for making
 * too-frequent job requests.
 */

bool banned_users::is_banned_user(

    string                                  orighost,
    string                                  user)

  {
  time_t current_time = time(NULL);
  pair<string, string> looked_up_user(orighost, user);
  map<pair<string, string>, list<time_t> >::iterator user_it;

  this->record_request_from_user(looked_up_user, current_time);

  // A user is banned if the difference in time between the most recent request
  // and the request made MAX_RECENT_REQUEST_TIMES times previously is smaller
  // than MAX_REQUEST_FREQUENCY.
  // e.g. If 5 requests have been made in the last 10 seconds, ban that user.
  if ((user_it = this->users.find(looked_up_user)) != this->users.end())
    {
    double difference = difftime(user_it->second.back(), user_it->second.front());

    if (difference < MAX_REQUEST_FREQUENCY)
      {
      return true;
      }
    }

  return false;
  }



