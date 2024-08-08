#include "Base64.h"
#include <cstring>
#include <algorithm>

using namespace std;

const static string table="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

string Base64::encode(const string& raw)
{
    std::string base64;

	unsigned char* p = (unsigned char*)raw.data();
	int count = raw.length() / 3 + 1;
	int cursor = 0;
	for (int i = 0; i < count; i++)
	{
		unsigned char first = 0x00;
		unsigned char second = 0x00;
		unsigned char third = 0x00;
		unsigned char fourth = 0x00;

		unsigned char tmp = 0x00;

		/* first charactor */
		first = *(p) >> 2;
		base64.push_back(table[first]);

		/* second charactor */
		tmp = *(p) << 6;
		tmp = tmp >> 2;
		second = second | tmp;
		tmp = 0x00;
		cursor++;
		if (cursor < raw.length())
		{
			p++;
			second = second | (*(p) >> 4);
			base64.push_back(table[second]);

			/* third charactor */
			tmp = *(p) << 4;
			tmp = tmp >> 2;
			third = third | tmp;
			tmp = 0x00;
			cursor++;
			if (cursor < raw.length())
			{
				p++;
				third = third | (*(p) >> 6);
				base64.push_back(table[third]);

				/* fourth charactor */
				tmp = *(p) << 2;
				tmp = tmp >> 2;
				fourth = fourth | tmp;
				tmp = 0x00;
				base64.push_back(table[fourth]);

				cursor++;
				if (cursor < raw.length())
				{
					p++;
				}
				else
				{
					break;
				}
			}
			else
			{
				fourth = table.length() - 1;

				base64.push_back(table[third]);
				base64.push_back(table[fourth]);
			}
		}
		else
		{
			third = table.length() - 1;
			fourth = third;

			base64.push_back(table[second]);
			base64.push_back(table[third]);
			base64.push_back(table[fourth]);
		}
	}

	return base64;
}

string Base64::decode(const string& base64)
{
    std::string raw;

	if (base64.length() % 4 == 0)
	{
		int count = base64.length() / 4;

		/* equal sign count */
		int equalCount = std::count(base64.begin(), base64.end(), '=');

		for (int i = 0; i < count; i++)
		{
			unsigned char first = 0x00;
			unsigned char second = 0x00;
			unsigned char third = 0x00;

			unsigned char tmp = 0x00;

			/* first charactor */
			tmp = table.find(base64.at(i * 4));
			tmp = tmp << 2;
			first = first | tmp;
			tmp = 0x00;
			tmp = table.find(base64.at(i * 4 + 1));
			tmp = tmp >> 4;
			first = first | tmp;
			tmp = 0x00;
			raw.push_back(first);

			/* second charactor */
			tmp = table.find(base64.at(i * 4 + 1));
			tmp = tmp << 4;
			second = second | tmp;
			tmp = 0x00;
			tmp = table.find(base64.at(i * 4 + 2));
			tmp = tmp >> 2;
			second = second | tmp;
			tmp = 0x00;
			raw.push_back(second);

			/* third charactor */
			tmp = table.find(base64.at(i * 4 + 2));
			tmp = tmp << 6;
			third = third | tmp;
			tmp = 0x00;
			tmp = table.find(base64.at(i * 4 + 3));
			third = third | tmp;
			tmp = 0x00;
			raw.push_back(third);

			if (i + 1 >= count)
			{
				switch (equalCount)
				{
				case 1:
					raw.pop_back();
					break;
				case 2:
					raw.pop_back();
					raw.pop_back();
					break;
				default:
					break;
				}
			}
		}
	}

	return raw;
}
