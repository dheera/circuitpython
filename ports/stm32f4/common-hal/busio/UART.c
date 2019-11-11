/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "shared-bindings/microcontroller/__init__.h"
#include "shared-bindings/busio/UART.h"

#include "mpconfigport.h"
#include "lib/utils/interrupt_char.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "supervisor/shared/translate.h"

#include "tick.h"
#include "stm32f4xx_hal.h" 

STATIC bool reserved_uart[MAX_UART];
int errflag; //Used to restart read halts

void uart_reset(void) {
    #ifdef USART1
        reserved_uart[0] = false;
        MP_STATE_PORT(cpy_uart_obj_all)[0] = NULL;
        __HAL_RCC_USART1_FORCE_RESET();
        __HAL_RCC_USART1_RELEASE_RESET();
        __HAL_RCC_USART1_CLK_DISABLE(); 
    #endif
    #ifdef USART2
        reserved_uart[1] = false;
        MP_STATE_PORT(cpy_uart_obj_all)[1] = NULL;
        __HAL_RCC_USART2_FORCE_RESET();
        __HAL_RCC_USART2_RELEASE_RESET();
        __HAL_RCC_USART2_CLK_DISABLE(); 
    #endif
    #ifdef USART3
        reserved_uart[2] = false;
        MP_STATE_PORT(cpy_uart_obj_all)[2] = NULL;
        __HAL_RCC_USART3_FORCE_RESET();
        __HAL_RCC_USART3_RELEASE_RESET();
        __HAL_RCC_USART3_CLK_DISABLE(); 
    #endif
    #ifdef UART4
        reserved_uart[3] = false;
        MP_STATE_PORT(cpy_uart_obj_all)[3] = NULL;
        __HAL_RCC_UART4_FORCE_RESET();
        __HAL_RCC_UART4_RELEASE_RESET();
        __HAL_RCC_UART4_CLK_DISABLE();
    #endif
    #ifdef UART5
        reserved_uart[4] = false;
        MP_STATE_PORT(cpy_uart_obj_all)[4] = NULL;
        __HAL_RCC_UART5_FORCE_RESET();
        __HAL_RCC_UART5_RELEASE_RESET();
        __HAL_RCC_UART5_CLK_DISABLE();
    #endif
    #ifdef USART6
        reserved_uart[5] = false;
        MP_STATE_PORT(cpy_uart_obj_all)[5] = NULL;
        __HAL_RCC_USART6_FORCE_RESET();
        __HAL_RCC_USART6_RELEASE_RESET();
        __HAL_RCC_USART6_CLK_DISABLE(); 
    #endif
}

STATIC USART_TypeDef * assign_uart_or_throw(busio_uart_obj_t *self, bool pin_eval, 
                                            int uart_index, bool uart_taken) {
    if(pin_eval) {
        //assign a root pointer pointer for IRQ
        MP_STATE_PORT(cpy_uart_obj_all)[uart_index] = self;
        return mcu_uart_banks[uart_index];
    } else {
        if (uart_taken) {
            mp_raise_ValueError(translate("Hardware in use, try alternative pins"));
        } else {
            mp_raise_ValueError(translate("Invalid UART pin selection"));
        }
    }
}


STATIC void uart_clk_irq_enable(busio_uart_obj_t *self, USART_TypeDef * USARTx) {
    #ifdef USART1
    if(USARTx==USART1) { 
        reserved_uart[0] = true;
        __HAL_RCC_USART1_FORCE_RESET();
        __HAL_RCC_USART1_RELEASE_RESET();
        __HAL_RCC_USART1_CLK_ENABLE();
        self->irq = USART1_IRQn;
    } 
    #endif
    #ifdef USART2
    if(USARTx==USART2) { 
        reserved_uart[1] = true;
        __HAL_RCC_USART2_FORCE_RESET();
        __HAL_RCC_USART2_RELEASE_RESET();
        __HAL_RCC_USART2_CLK_ENABLE();
        self->irq = USART2_IRQn;
    } 
    #endif
    #ifdef USART3
    if(USARTx==USART3) { 
        reserved_uart[2] = true;
        __HAL_RCC_USART3_FORCE_RESET();
        __HAL_RCC_USART3_RELEASE_RESET();
        __HAL_RCC_USART3_CLK_ENABLE();
        self->irq = USART3_IRQn;
    } 
    #endif
    #ifdef UART4
    if(USARTx==UART4) { 
        reserved_uart[3] = true;
        __HAL_RCC_UART4_FORCE_RESET();
        __HAL_RCC_UART4_RELEASE_RESET();
        __HAL_RCC_UART4_CLK_ENABLE();
        self->irq = UART4_IRQn;
    } 
    #endif
    #ifdef UART5
    if(USARTx==UART5) { 
        reserved_uart[4] = true;
        __HAL_RCC_UART5_FORCE_RESET();
        __HAL_RCC_UART5_RELEASE_RESET();
        __HAL_RCC_UART5_CLK_ENABLE();
        self->irq = UART5_IRQn;
    } 
    #endif
    #ifdef USART6
    if(USARTx==USART6) { 
        reserved_uart[5] = true;
        __HAL_RCC_USART6_FORCE_RESET();
        __HAL_RCC_USART6_RELEASE_RESET();
        __HAL_RCC_USART6_CLK_ENABLE();
        self->irq = USART6_IRQn;
    } 
    #endif
}

void common_hal_busio_uart_construct(busio_uart_obj_t *self,
        const mcu_pin_obj_t * tx, const mcu_pin_obj_t * rx, uint32_t baudrate,
        uint8_t bits, uart_parity_t parity, uint8_t stop, mp_float_t timeout,
        uint16_t receiver_buffer_size) {

    //match pins to UART objects
    USART_TypeDef * USARTx;

    uint8_t tx_len = sizeof(mcu_uart_tx_list)/sizeof(*mcu_uart_tx_list);
    uint8_t rx_len = sizeof(mcu_uart_rx_list)/sizeof(*mcu_uart_rx_list);
    bool uart_taken = false;
    
    //Can have both pins, or either
    if ((tx != mp_const_none) && (rx != mp_const_none)) {
        //normal find loop if both pins exist
        for(uint i=0; i<tx_len;i++) {
            if (mcu_uart_tx_list[i].pin == tx) {
                //rx
                for(uint j=0; j<rx_len;j++) {
                    if (mcu_uart_rx_list[j].pin == rx 
                        && mcu_uart_rx_list[j].uart_index == mcu_uart_tx_list[i].uart_index) {
                        //keep looking if the UART is taken, edge case
                        if(reserved_uart[mcu_uart_tx_list[i].uart_index-1]) {
                            uart_taken = true;
                            continue;
                        }
                        //store pins if not
                        self->tx = &mcu_uart_tx_list[i];
                        self->rx = &mcu_uart_rx_list[j];
                        break;
                    }
                }
            }
        }
        USARTx = assign_uart_or_throw(self, (self->tx!=NULL && self->rx!=NULL), 
            self->tx->uart_index-1, uart_taken);
    } else if (tx==mp_const_none) {
        //If there is no tx, run only rx
        for(uint i=0; i<rx_len;i++) {
            if (mcu_uart_rx_list[i].pin == rx) {
                //keep looking if the UART is taken, edge case
                if(reserved_uart[mcu_uart_rx_list[i].uart_index-1]) {
                    uart_taken = true;
                    continue;
                }
                //store pins if not
                self->rx = &mcu_uart_rx_list[i];
                break;
            }
        }
        USARTx = assign_uart_or_throw(self, (self->rx!=NULL), 
            self->rx->uart_index-1, uart_taken);
    } else if (rx==mp_const_none) {
        //If there is no rx, run only tx
        for(uint i=0; i<tx_len;i++) {
            if (mcu_uart_tx_list[i].pin == tx) {
                //keep looking if the UART is taken, edge case
                if(reserved_uart[mcu_uart_tx_list[i].uart_index-1]) {
                    uart_taken = true;
                    continue;
                }
                //store pins if not
                self->tx = &mcu_uart_tx_list[i];
                break;
            }
        }
        USARTx = assign_uart_or_throw(self, (self->tx!=NULL), 
            (self->tx->uart_index-1), uart_taken);
    } else {
        //both pins cannot be empty
        mp_raise_ValueError(translate("Supply at least one UART pin"));
    }

    //Other errors
    if ( receiver_buffer_size == 0 ) {
        mp_raise_ValueError(translate("Invalid buffer size"));
    }
    if ( bits != 8 && bits != 9 ) {
        mp_raise_ValueError(translate("Invalid word/bit length"));
    }

    //GPIO Init
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (self->tx!=NULL) {
        GPIO_InitStruct.Pin = pin_mask(tx->number);
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = self->tx->altfn_index;
        HAL_GPIO_Init(pin_port(tx->port), &GPIO_InitStruct);
    }
    if (self->rx!=NULL) {
        GPIO_InitStruct.Pin = pin_mask(rx->number);
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = self->rx->altfn_index; 
        HAL_GPIO_Init(pin_port(rx->port), &GPIO_InitStruct);
    }

    uart_clk_irq_enable(self,USARTx);

    self->handle.Instance = USARTx;
    self->handle.Init.BaudRate = baudrate;
    self->handle.Init.WordLength = (bits == 9) ? UART_WORDLENGTH_9B : UART_WORDLENGTH_8B;
    self->handle.Init.StopBits = (stop>1) ? UART_STOPBITS_2 : UART_STOPBITS_1;
    self->handle.Init.Parity = (parity==PARITY_ODD) ? UART_PARITY_ODD : 
                               (parity==PARITY_EVEN) ? UART_PARITY_EVEN :
                               UART_PARITY_NONE;
    self->handle.Init.Mode = (self->tx != NULL && self->rx != NULL) ? UART_MODE_TX_RX :
                             (self->tx != NULL) ? UART_MODE_TX :
                             UART_MODE_RX;
    self->handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    self->handle.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&self->handle) != HAL_OK)
    {
        mp_raise_ValueError(translate("UART Init Error"));

    }

    // Init buffer for rx and claim pins
    if (self->rx != NULL) {
        ringbuf_alloc(&self->rbuf, receiver_buffer_size, true);
        if (!self->rbuf.buf) {
            mp_raise_ValueError(translate("UART Buffer allocation error"));
        }
        claim_pin(rx);
    }
    if (self->tx != NULL) {
        claim_pin(tx);
    }
    self->baudrate = baudrate;
    self->timeout_ms = timeout * 1000;

    //start the interrupt series
    if ((HAL_UART_GetState(&self->handle) & HAL_UART_STATE_BUSY_RX) == HAL_UART_STATE_BUSY_RX) {
        mp_raise_ValueError(translate("Could not start interrupt, RX busy"));
    }

    //start the receive interrupt chain
    HAL_NVIC_DisableIRQ(self->irq); //prevent handle lock contention
    HAL_UART_Receive_IT(&self->handle, &self->rx_char, 1);
    HAL_NVIC_SetPriority(self->irq, UART_IRQPRI, UART_IRQSUB_PRI);
    HAL_NVIC_EnableIRQ(self->irq);

    errflag = HAL_OK;
}

bool common_hal_busio_uart_deinited(busio_uart_obj_t *self) {
    return self->tx->pin == mp_const_none;
}

void common_hal_busio_uart_deinit(busio_uart_obj_t *self) {
    if(common_hal_busio_uart_deinited(self)) return;
    
    reset_pin_number(self->tx->pin->port,self->tx->pin->number);
    reset_pin_number(self->rx->pin->port,self->rx->pin->number);
    self->tx = mp_const_none;
    self->rx = mp_const_none;
    gc_free(self->rbuf.buf);
    self->rbuf.size = 0;
    self->rbuf.iput = self->rbuf.iget = 0;
}

size_t common_hal_busio_uart_read(busio_uart_obj_t *self, uint8_t *data, size_t len, int *errcode) {
    if (self->rx == NULL) {
        mp_raise_ValueError(translate("No RX pin"));
    }

    size_t rx_bytes = 0;
    uint64_t start_ticks = ticks_ms;

    // Wait for all bytes received or timeout, same as nrf
    while ( (ringbuf_count(&self->rbuf) < len) && (ticks_ms - start_ticks < self->timeout_ms) ) {
        RUN_BACKGROUND_TASKS;
        //restart if it failed in the callback
        if(errflag != HAL_OK) {
            errflag = HAL_UART_Receive_IT(&self->handle, &self->rx_char, 1);
        }
        // Allow user to break out of a timeout with a KeyboardInterrupt.
        if ( mp_hal_is_interrupted() ) {
            return 0;
        }
    }

    // Halt reception
    HAL_NVIC_DisableIRQ(self->irq);
    // copy received data
    rx_bytes = ringbuf_count(&self->rbuf);
    //Used for debugging
    //mp_printf(&mp_plat_print, "Read: count:%d, buffer location%p, er:%d\n", rx_bytes, &self->rbuf,errflag);
    rx_bytes = MIN(rx_bytes, len);
    for ( uint16_t i = 0; i < rx_bytes; i++ ) {
        data[i] = ringbuf_get(&self->rbuf);
    }
    HAL_NVIC_EnableIRQ(self->irq);

    if (rx_bytes == 0) {
        *errcode = EAGAIN;
        return MP_STREAM_ERROR;
    }
    return rx_bytes; 
}

// Write characters.
size_t common_hal_busio_uart_write(busio_uart_obj_t *self, const uint8_t *data, size_t len, int *errcode) {
    if (self->tx == NULL) {
        mp_raise_ValueError(translate("No TX pin"));
    }

    HAL_NVIC_DisableIRQ(self->irq);

    if (HAL_UART_Transmit(&self->handle, (uint8_t *)data, len, self->timeout_ms) != HAL_OK) {
        mp_raise_ValueError(translate("UART write error"));
    }

    HAL_UART_Receive_IT(&self->handle, &self->rx_char, 1);
    HAL_NVIC_SetPriority(self->irq, UART_IRQPRI, UART_IRQSUB_PRI);
    HAL_NVIC_EnableIRQ(self->irq);

    return len;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *handle)
{
    for(int i=0; i<7; i++) {
        //get context pointer and cast it as struct pointer
        busio_uart_obj_t * context = (busio_uart_obj_t *)MP_STATE_PORT(cpy_uart_obj_all)[i];
        if(handle == &context->handle) {
            //check if transaction is ongoing
            if((HAL_UART_GetState(handle) & HAL_UART_STATE_BUSY_RX) == HAL_UART_STATE_BUSY_RX) {
                return;
            }
            ringbuf_put_n(&context->rbuf, &context->rx_char, 1);
            errflag = HAL_UART_Receive_IT(handle, &context->rx_char, 1);
            
            return;
        }
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *UartHandle)
{
    if (__HAL_UART_GET_FLAG(UartHandle, UART_FLAG_PE) != RESET) {
        __HAL_UART_CLEAR_PEFLAG(UartHandle);
    } else if (__HAL_UART_GET_FLAG(UartHandle, UART_FLAG_FE) != RESET) {
        __HAL_UART_CLEAR_FEFLAG(UartHandle);
    } else if (__HAL_UART_GET_FLAG(UartHandle, UART_FLAG_NE) != RESET) {
        __HAL_UART_CLEAR_NEFLAG(UartHandle);
    } else if (__HAL_UART_GET_FLAG(UartHandle, UART_FLAG_ORE) != RESET) {
        __HAL_UART_CLEAR_OREFLAG(UartHandle);
    }
    //restart serial read after an error
    for(int i=0; i<7; i++) {
        busio_uart_obj_t * context = (busio_uart_obj_t *)MP_STATE_PORT(cpy_uart_obj_all)[i];
        if(UartHandle == &context->handle) {
            HAL_UART_Receive_IT(UartHandle, &context->rx_char, 1);
            return;
        }
    }

}

uint32_t common_hal_busio_uart_get_baudrate(busio_uart_obj_t *self) {
    return self->baudrate;
}

void common_hal_busio_uart_set_baudrate(busio_uart_obj_t *self, uint32_t baudrate) {
    //Don't reset if it's the same value
    if (baudrate == self->baudrate) return;

    //Otherwise de-init and set new rate
    if(HAL_UART_DeInit(&self->handle) != HAL_OK) {
        mp_raise_ValueError(translate("UART De-init error"));
    }
    self->handle.Init.BaudRate = baudrate;
    if(HAL_UART_Init(&self->handle) != HAL_OK) {
        mp_raise_ValueError(translate("UART Re-init error"));
    }

    self->baudrate = baudrate;
}

uint32_t common_hal_busio_uart_rx_characters_available(busio_uart_obj_t *self) {
    return ringbuf_count(&self->rbuf);
}

void common_hal_busio_uart_clear_rx_buffer(busio_uart_obj_t *self) {
    // Halt reception
    HAL_NVIC_DisableIRQ(self->irq);
    ringbuf_clear(&self->rbuf);
    HAL_NVIC_EnableIRQ(self->irq);
}

bool common_hal_busio_uart_ready_to_tx(busio_uart_obj_t *self) {
    return __HAL_UART_GET_FLAG(&self->handle,UART_FLAG_TXE);
}

STATIC void call_hal_irq(int uart_num) {
    //Create casted context pointer
    busio_uart_obj_t * context = (busio_uart_obj_t *)MP_STATE_PORT(cpy_uart_obj_all)[uart_num-1];
    if(context != NULL) {
        HAL_NVIC_ClearPendingIRQ(context->irq);
        HAL_UART_IRQHandler(&context->handle);
    }
}

// UART/USART IRQ handlers
void USART1_IRQHandler(void) {
    call_hal_irq(1);
}

void USART2_IRQHandler(void) {
    call_hal_irq(2);
}

void USART3_IRQHandler(void) {
    call_hal_irq(3);
}

void UART4_IRQHandler(void) {
    call_hal_irq(4);
}

void UART5_IRQHandler(void) {
    call_hal_irq(5);
}

void USART6_IRQHandler(void) {
    call_hal_irq(6);
}
