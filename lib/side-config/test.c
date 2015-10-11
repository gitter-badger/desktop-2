//Public Domain (CC0) tool to test the side-config lib

#include <side/config.h>

int main(void)
{
  side_dump_config_file("lib/side-config/test.conf");
  side_lookup_value("lib/side-config/test.conf", "a");
  side_set_value("lib/side-config/test.conf", "ABC", "123", true);
  return 0;
}
