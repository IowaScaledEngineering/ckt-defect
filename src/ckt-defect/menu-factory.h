#ifndef MENU_FACTORY_H
#define MENU_FACTORY_H

#include <memory>
#include "src/menu/menu.h"
#include "configuration.h"
#include "display-lcd.h"

/**
 * @brief Builds the hierarchical menu tree for the Defect Detector.
 * @param cfg Reference to the current detector configuration structure.
 * @param lcd Pointer to the initialized LCD display driver instance.
 * @return std::shared_ptr<Menu> The generic root base menu object.
 */
std::shared_ptr<Menu> createAppMenu(DetectorConfiguration &cfg, DisplayLcd *lcd);

#endif // MENU_FACTORY_H