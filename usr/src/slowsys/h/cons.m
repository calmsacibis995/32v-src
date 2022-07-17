#
# VAX console interface registers
#

	.set	RXCS_IE,0x40	# receiver interrupt enable
	.set	RXCS_DONE,0x80	# receiver done
	.set	RXDB_DATA,0xff	# received character
	.set	RXDB_ID,0xf00	# channel id
	.set	RXDB_ERR,0x80000000	# receiver error

	.set	TXCS_IE,0x40	# transmitter interrupt i"enable
	.set	TXCS_RDY,0x80	# transmitter ready for next char
	.set	TXCS_BRDY,7		# bit number of TXCS_RDY
	.set	TXDB_DATA,0xff	# transmitter byte
	.set	TXDB_ID,0xf00	# channel id
