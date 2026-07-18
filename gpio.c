#include "gpio.h"
#include "rcc.h"

void gpio_init(GPIO_TypeDef *port, uint8_t pin, uint8_t mode)
{
    rcc_enable(port);
if(mode== IN_PU){
	port->ODR |=(1<<pin);
}
    uint8_t offset;

    if (pin < 8) {
        offset = pin * 4;
        port->CRL &= ~(0xF << offset);
        port->CRL |= (mode << offset);
    } else {
        offset = (pin - 8) * 4;
        port->CRH &= ~(0xF << offset);
        port->CRH |= (mode << offset);
    }
}

void gpio_write(GPIO_TypeDef *port, uint8_t pin, uint8_t value)
{
    if (value)
        port->BSRR = (1 << pin);
    else
        port->BSRR = (1 << (pin + 16));
}

uint8_t gpio_read(GPIO_TypeDef *port, uint8_t pin)
{
    if (port->IDR & (1 << pin))
        return 1;
    else
        return 0;
}

void gpio_toggle(GPIO_TypeDef *port, uint8_t pin)
{
    if (port->ODR & (1 << pin))
        port->BSRR = (1 << (pin + 16));
    else
        port->BSRR = (1 << pin);
}
