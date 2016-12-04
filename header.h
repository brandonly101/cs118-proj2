#include <stdint.h>
#include <vector>
#include <iostream>
using namespace std;

// Global Constants
const uint16_t HEADER_SIZE = 8; // 8 bytes
const uint16_t MSS = 1024;
const uint16_t MAX_PACKET_LEN = 1032; // max 1024 bytes of payload
const uint16_t MSN = 30720; // 30 KB max sequence number
const uint16_t MAX_RECVWIN = 15360; // TODO 

class Header {
	public:
		Header() {
			m_seqNum = 0;
			m_ackNum = 0;
			m_window = 0;
			m_ack = 0;
			m_syn = 0;
			m_fin = 0;
		}

		Header(uint16_t seqNum, uint16_t ackNum, uint16_t window, bool ack, bool syn, bool fin) {
			m_seqNum = seqNum;
			m_ackNum = ackNum;
			m_window = window;
			m_ack = ack;
			m_syn = syn;
			m_fin = fin;
		}

		uint16_t getSeqNum() {
			return m_seqNum;
		}

		uint16_t getAckNum() {
			return m_ackNum;
		}

		uint16_t getWindow() {
			return m_window;
		}

		bool isAck() {
			return m_ack;
		}

		bool isSyn() {
			return m_syn;
		}

		bool isFin() {
			return m_fin;
		}

		uint16_t encodeFlags() {
			uint16_t res;
			if (!m_ack && !m_syn && !m_fin)
				res = 0;
			else if (!m_ack && !m_syn && m_fin)
				res = 1;
			else if (!m_ack && m_syn && !m_fin)
				res = 2;
			else if (!m_ack && m_syn && m_fin)
				res = 3;
			else if (m_ack && !m_syn && !m_fin)
				res = 4;
			else if (m_ack && !m_syn && m_fin)
				res = 5;
			else if (m_ack && m_syn && !m_fin)
				res = 6;
			else if (m_ack && m_syn && m_fin)
				res = 7;
			return res;
		}

		uint8_t encodeFront16(uint16_t num) {
			return (uint8_t) num;
		}

		uint8_t encodeBack16(uint16_t num) {
			uint16_t shift = num >> 8;
			return (uint8_t) shift;
		}

		uint16_t decodeNum(uint8_t front, uint8_t back) {
			uint16_t first = (uint16_t) front;
			uint16_t second = (uint16_t) back;
			second = second << 8;
			return first + second;
		}

		bool findAck(uint8_t num) {
			int x = (uint16_t) num;
			if (x >= 4 && x <= 7)
				return true;
			return false;
		}

		bool findSyn(uint8_t num) {
			int x = (uint16_t) num;
			if (x == 2 || x == 3 || x == 6 || x == 7)
				return true;
			return false;
		}

		bool findFin(uint8_t num) {
			int x = (uint16_t) num;
			if (x%2 == 1)
				return true;
			return false;
		}


		vector<char> encode() {
			vector<char> res;
			uint8_t z = 0;
			res.push_back(encodeFront16(m_seqNum));
			res.push_back(encodeBack16(m_seqNum));
			res.push_back(encodeFront16(m_ackNum));
			res.push_back(encodeBack16(m_ackNum));
			res.push_back(encodeFront16(m_window));
			res.push_back(encodeBack16(m_window));
			res.push_back(z);
			res.push_back(encodeFront16(encodeFlags()));
			return res;
		}

		void decode(vector<char> head) {
			m_seqNum = decodeNum(head[0], head[1]);
			m_ackNum = decodeNum(head[2], head[3]);
			m_window = decodeNum(head[4], head[5]);
			m_ack = findAck(head[7]);
			m_syn = findSyn(head[7]);
			m_fin = findFin(head[7]);
			cout << m_seqNum << endl;
			cout << m_ackNum << endl;
			cout << m_window << endl;
			cout << m_ack << endl;
			cout << m_syn << endl;
			cout << m_fin << endl;
		}

	private:
		uint16_t m_seqNum;
		uint16_t m_ackNum;
		uint16_t m_window;
		bool m_ack;
		bool m_syn;
		bool m_fin;
};