gpio_init(GPIOX,pin,模式);对引脚配置输入输出模式 gpio_init(GPIOA,0,IN_PU),对按键所接引脚设置上拉输入模式
gpio_write(GPIOX,pin,0/1);对引脚写0/1  gpio_write(GPIOC,13,1)对pc13led灯设置状态
gpio_toggle(GPIOX,pin);对引脚进行状态翻转 gpio_toggle(GPIOC,13)对led状态进行翻转，亮-灭，灭-亮
gpio_read(GPIOX,pin)；读某一位引脚的状态0/1 gpio_read(GPIOA,0)读对应引脚电平

usart_init(USARTX,baud);选择外设并设置波特率 usart_init（USART1，115200）；
usart_send_byte(USARTX,data)往USARTX里写data  usart_send_byte(USART2, b); 
usart_receive_byte(USARTX)从USARTX里接收data，有阻塞，一直死等，有数据再往下一步走 uint8_t b = usart_receive_byte(USART1);
usart_send_string(USARTX,字符串)；一个一个字节得发 usart_send_string(USART2,"AT\r\n");
usart_try_receive(USARTX,字节地址)；每次去看寄存器里有没有数据不用一直等，有就从这个地址里把数据收走，没有就走
有返回值，有数据返回1，没数据返回0 usart_try_receive(USART2, &b)
usart2_init（baud）；usart2_init(115200);

systick_delay_ms(延时)延时 systick_delay_ms(20)延时20ms
systick_delay_us(延时); 


exti_init(GPIOX，pin，上升沿触发或下降沿触发)，对GPIOX的某一个引脚进行配置，允许这个引脚触发的中断进入CPU button_flag — 中断发生后置 1，主循 if(button_flag){...; button_flag=0;}
exti_init(GPIOA,0,EXTI_FALLING);
上升沿：EXTI_RISING 下降沿：EXTI_FALLING

timer_pwm_init(频率，初始占空比)  timer_pwm_init(1000,0);「频率定死后只能改占空比」
timer_pwm_set_duty(占空比)设置占空比）duty范围0~100 timer_pwm_set_duty(50);根据占空比的不断变化实现亮度的变化
PWM 固定从 PA1（TIM2_CH2）输出，引脚写死在函数里，不是参数。

i2c_init（）； 
i2c_send_byte(挑外设（I2C1...）,地址，data);   i2c_send_byte(I2C1,0x23,0x10);
i2c_read_bytes(挑外设,地址，字节数，数组);  i2c_read_bytes(I2C1, 0x23, 2, buf);