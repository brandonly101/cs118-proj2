#include <stdint.h>
#include <vector>
#include <iostream>
using namespace std;

class header {
	public:
		header() {
			m_seqNum = 0;
			m_ackNum = 0;
			m_window = 0;
			m_ack = 0;
			m_syn = 0;
			m_fin = 0;
		}

		header(uint16_t seqNum, uint16_t ackNum, uint16_t window, bool ack, bool syn, bool fin) {
			m_seqNum = seqNum;
			m_ackNum = ackNum;
			m_window = window;
			m_ack = ack;
			m_syn = syn;
			m_fin = fin;
		}

		uint16_t returnSeqNum() {
			return m_seqNum;
		}

		uint16_t returnAckNum() {
			return m_ackNum;
		}

		uint16_t returnWindow() {
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

		uint8_t encodeFlags (){
			if (!m_ack && !m_syn && !m_fin)
				return 0;
			if (!m_ack && !m_syn && m_fin)
				return 1;
			if (!m_ack && m_syn && !m_fin)
				return 2;
			if (!m_ack && m_syn && m_fin)
				return 3;
			if (m_ack && !m_syn && !m_fin)
				return 4;
			if (m_ack && !m_syn && m_fin)
				return 5;
			if (m_ack && m_syn && !m_fin)
				return 6;
			if (m_ack && m_syn && m_fin)
				return 7;
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

		bool findSys(uint8_t num) {
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
			m_syn = findSys(head[7]);
			m_fin = findSys(head[7]);
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