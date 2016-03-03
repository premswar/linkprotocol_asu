APP :
BYTE[0],BYTE[1] : packet length

LINK :
    BYTE[0] : 
        - b7 : message type (if b7 == 0 : data packet, b7 ==1 : control packet);
        - b6 : retransmission frame (b6 == 0 : first tx, b6 == 1 : retransmission);
        - b1 : START_FRAME (if b1 == 1)
        - b0 : END_FRAME (if b0 == 1)

    Data Frame :
        BYTE[1] : sequence number
        BYTE[2] ~ BYTE[95]

    Control Frame :
        BYTE[1] : control field counter (==n)
        BYTE[2] ~ BYTE[95]
            control field : (b15==0: ack, b15==1:nack)
                            (b14~b0: sequence number) 
                   

