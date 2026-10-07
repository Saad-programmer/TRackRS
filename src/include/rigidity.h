// Creator: Saad AIT YAHIA - @github: Saad-programmer
#pragma once

#include <map>

class PRigidity {
public:
  PRigidity();
  float getRigidity(std::string sprite) const;
private:
  PRigidity(const PRigidity&);
  PRigidity& operator=(const PRigidity&);

  // Core data containing sprite name value pairs
  std::map<std::string, float> rigiditymap;
};
