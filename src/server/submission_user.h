class banned_users
  {
  public:
  banned_users();
  ~banned_users();

  void record_request_from_user(std::string orighost, std::string user, time_t current_time);
  bool is_banned_user(std::string orighost, std::string user);

  private:
  std::set<std::pair<std::string, std::string>> banned_users;

  }

