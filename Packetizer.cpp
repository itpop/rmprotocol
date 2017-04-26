/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE:     Packetizer.cpp
--
-- PROGRAM:         RMProtocol
--
-- Functions
--                  vector<std::string> parketize();
--                  uint16_t calculateCRC16(const std::string& data);
--                  string CRCtoString(uint16_t crc);

--
-- DATE:            December 3, 2016
--
-- DESIGNER:        Trista Huang, Isaac Morneau
--
-- PROGRAMMER:      Trista Huang, Isaac Morneau
--
-- NOTES:
-- This class is designed to packetize a single text string (read from file) into individual packets
-- based on the defined packet size.
----------------------------------------------------------------------------------------------------------------------*/
#include "Packetizer.h"
using namespace std;

size_t fileSize;
string rawStr;

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:        parketize
--
-- DATE:            December 3, 2016
--
-- DESIGNER:        Trista Huang
--
-- PROGRAMMER:      Trista Huang
--
-- INTERFACE:       parketize()
--
-- RETURNS:         vector<string> - a vector of packetized strings
--
-- NOTES:
-- Takes a single text string (read from file) and splits it into individual packets, based on the
-- defined packet size. A SYNC byte is inserted at the beginning of each packet, followed by 1024 
-- bytes of data, and 2 bytes of checksum. If the size of data is less than 1024, then it's filled
-- with NULs.
----------------------------------------------------------------------------------------------------------------------*/
vector<string> parketize() {
    // check sum
    int sum;
    // checksum (unsigned int)
    uint16_t crcRaw;
    // 2 bytes of crc
    string	crcChars;
    // the data to be calculated
    string data;
    BOOL succeeded = false;
    BOOL completed = false;

    // remaining string
	size_t remaining = fileSize;
    // string offset for next packet
    int offset = 0;
    // packet starting index (0+1SYN = 1)
    int index = PACKET_DATA_INDEX;

    try {
        write_packets.clear();

	    while (!completed) {

		    // reset checksum to 0
		    sum = 0;
            TCHAR packet[PACKET_SIZE] = "";
            packet[0] = SYN;

		    // check for last packet
		    if (remaining <= PACKET_DATA_SIZE) {
			    for (size_t i = rawStr.length() - remaining; i < rawStr.length(); i++) {
				    packet[index++] = rawStr[i];
				    remaining--;
				    // if no more data, add CHECKSUM
				    if (remaining == 0) {
					    // calculate checksum and add it to the packet here
                        data = rawStr.substr(offset, PACKET_DATA_SIZE);
                        crcRaw = calculateCRC16(data);
                        crcChars = CRCtoString(crcRaw);

                        // NUL can not be the filler as the packets that contain NUL, NULL, or '\0' are simply
                        // terminated when transmitting. To make up a fixed length packet, we chose NUL0 as the filler.
                        for (size_t j = index; j <= PACKET_DATA_SIZE; j++)
                            packet[j] = NUL0;

                        packet[PACKET_SIZE - 2] = crcChars[0];
                        packet[PACKET_SIZE - 1] = crcChars[1];

					    write_packets.push_back(packet);
					    completed = true;
					    break;
				    }
			    }
		    }
		    else {
			    // loop through string character by character to wrap packet
			    for (size_t i = offset; i < rawStr.length(); i++) {
				    packet[index++] = rawStr[i];
				    remaining--;

				    // once index reaches PACKET_DATA_SIZE, break the loop
				    if (index > PACKET_DATA_SIZE) {
					    succeeded = true;
					    break;
				    }
			    }
		    }

		    if (succeeded) {
			    // calculate the checksum and add it to the packet here
                data = rawStr.substr(offset, PACKET_DATA_SIZE);
                crcRaw = calculateCRC16(data);
                crcChars = CRCtoString(crcRaw);
                packet[PACKET_SIZE - 2] = crcChars[0];
                packet[PACKET_SIZE - 1] = crcChars[1];

			    // stores into vector of packets
			    write_packets.push_back(packet);

                // reset packet data index and offset
                index = PACKET_DATA_INDEX;
                offset += PACKET_DATA_SIZE;
			    succeeded = false;
		    }
	    }
    }
    catch (exception& e) {
        OutputDebugString(e.what());
    }

	return write_packets;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:        CRCtoString
--
-- DATE:            December 3, 2016
--
-- DESIGNER:        Isaac Morneau
--
-- PROGRAMMER:      Isaac Morneau
--
-- INTERFACE:       CRCtoString(uint16_t crc)
--                  crc - unsigned int representing crc value
--
-- RETURNS:         Returns 2 bytes of CRC checksum
--
-- NOTES:
-- Returns 2 bytes of CRC checksum.
----------------------------------------------------------------------------------------------------------------------*/
string CRCtoString(uint16_t crc) {
    return{ static_cast<char>(crc >> 8), static_cast<char>(0x00FF & crc) };
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:        calculateCRC16
--
-- DATE:            December 3, 2016
--
-- DESIGNER:        Isaac Morneau
--
-- PROGRAMMER:      Isaac Morneau
--
-- INTERFACE:       calculateCRC16(const string& data)
--                  data - a const string data to be calculated
--
-- RETURNS:         unsigned int of CRC of characters using 16 bit polynomial
--
-- NOTES:
-- Computes the CRC of characters using the CRC 16 bit polynomial X^16 + X^15 + X^2 + 1.
----------------------------------------------------------------------------------------------------------------------*/
uint16_t calculateCRC16(const string& data) {
    static constexpr auto poly = 0x8005;
    auto size = data.size();
    uint16_t out = 0;
    int bits_read = 0;
    bool bit_flag;

    vector<char> bytes(data.begin(), data.end());

    int i = 0;
    while (size > 0) {
        bit_flag = (out >> 15) != 0;

        /* Get next bit: */
        // work from the least significant bits
        out = (out << 1) | ((bytes[i] >> bits_read) & 1);

        /* Increment bit counter: */
        if (++bits_read > 7) {
            bits_read = 0;
            i++;
            size--;
        }

        /* Cycle check: */
        if (bit_flag) {
            out ^= poly;
        }
    }

    // "push out" the last 16 bits
    for (int i = 0; i < 16; ++i) {
        out = (out << 1) ^ (poly * ((out >> 15) != 0));
    }

    //reverse the bits
    uint16_t crc = 0;
    for (int i = 0x8000, j = 0x001; i; i >>= 1, j <<= 1) {
        if (i & out) {
            crc |= j;
        }
    }
    return crc;
}