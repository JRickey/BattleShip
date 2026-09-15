import sys
import os
from pathlib import Path


def write_if_changed(path, text):
	data = text.encode('utf-8')
	output = Path(path)
	if not output.exists() or output.read_bytes() != data:
		output.write_bytes(data)

DECODE_PRINT_MARGIN = 35

CHAR_A = 0x0
CHAR_Z = 0x19
CHAR_a = 0x1a
CHAR_z = 0x33
CHAR_9 = 0x35
CHAR_0 = 0x3e

WHITESPACE = 0xffffffdf
LINE_BREAK = 0xffffffc9

encodeMapParagraphFont = {
	":": 0x34,
	"9": 0x35,
	"8": 0x36,
	"7": 0x37,
	"6": 0x38,
	"5": 0x39,
	"4": 0x3a,
	"3": 0x3b,
	"2": 0x3c,
	"1": 0x3d,
	"0": 0x3e,
	".": 0x3f,
	"-": 0x40,
	",": 0x41,
	"&": 0x42,
	'"': 0x43,
	"/": 0x44,
	"'": 0x45,
	"?": 0x46,
	"(": 0x47,
	")": 0x48,
	"é": 0x49,
}
decodeMapParagraphFont = { v: k for k, v in encodeMapParagraphFont.items() }
encodeMapTitleFont = {
 	".": 0x34,
 	",": 0x35,
 	"'": 0x36,
 	"4": 0x37,
}
decodeMapTitleFont = { v: k for k, v in encodeMapTitleFont.items() }

# Staff and titles (title font):
# 0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f  10 11 12 13 14 15 16 17 18 19
# A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W  X  Y  Z
# 1a 1b 1c 1d 1e 1f 20 21 22 23 24 25 26 27 28 29 2a 2b 2c 2d 2e 2f 30 31 32 33
# a  b  c  d  e  f  g  h  i  j  k  l  m  n  o  p  q  r  s  t  u  v  w  x  y  z
# 34 35 36 37
# .  ,  '  4

# Info and companies (paragraph font):
# 0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f  10 11 12 13 14 15 16 17 18 19
# A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W  X  Y  Z
# 1a 1b 1c 1d 1e 1f 20 21 22 23 24 25 26 27 28 29 2a 2b 2c 2d 2e 2f 30 31 32 33
# a  b  c  d  e  f  g  h  i  j  k  l  m  n  o  p  q  r  s  t  u  v  w  x  y  z
# 34 35 36 37 38 39 3a 3b 3c 3d 3e 3f 40 41 42 43 44 45 46 47 48 49
# :  9  8  7  6  5  4  3  2  1  0  .  -  ,  &  "  /  '  ?  (  )  é


def encode(text, encodingMap):
	output = ""
	for char in text:
		if char == '\n':
			continue
		if char == ' ':
			output += hex(WHITESPACE)
		elif char in encodingMap.keys():
			output += hex(encodingMap[char])
		elif ord(char) >= ord('A') and ord(char) <= ord('Z'):
			output += hex(ord(char) - ord('A') + CHAR_A)
		elif ord(char) >= ord('a') and ord(char) <= ord('z'):
			output += hex(ord(char) - ord('a') + CHAR_a)
		else:
			print(f"Character '{char}' not available in font", file=sys.stderr)
		output += ','
	output += '\n'
	return output


def encodeMultiline(text, encodingMap):
	output = ""
	for char in text.replace('\n\n', '\n'):
		if char == '\n':
			output += hex(LINE_BREAK)
		elif char == ' ':
			output += hex(WHITESPACE)
		elif char in encodingMap.keys():
			output += hex(encodingMap[char])
		elif ord(char) >= ord('A') and ord(char) <= ord('Z'):
			output += hex(ord(char) - ord('A') + CHAR_A)
		elif ord(char) >= ord('a') and ord(char) <= ord('z'):
			output += hex(ord(char) - ord('a') + CHAR_a)
		else:
			print(f"Character '{char}' not available in font", file=sys.stderr)
		output += ','
	output += '\n'
	return output


def decode(text, decodingMap):
	wordSize = 16
	word = []
	for hexString in text.replace('\n', '').split(','):
		value = eval(hexString)
		word.append(value)

		if value == WHITESPACE:
			print(' ', end='')

		elif value in decodingMap.keys():
			print(decodingMap[value], end='')
		elif value >= CHAR_A and value <= CHAR_Z:
			print(chr(value + ord('A')), end='')
		elif value >= CHAR_a and value <= CHAR_z:
			print(chr(value - CHAR_a + ord('a')), end='')
		else:
			print(f"Unknown index: '{hexString}'", file=sys.stderr)

		if len(word) == wordSize:
			print(' '*(max(DECODE_PRINT_MARGIN-len(word), 1)), end='')
			print(",".join([hex(x) for x in word]))
			word = []

	print(' '*(max(DECODE_PRINT_MARGIN-len(word), 1)), end='')
	print(",".join([hex(x) for x in word]))


def decodeMultiline(text, decodingMap):
	word = []
	for hexString in text.replace('\n', '').split(','):
		value = eval(hexString)
		word.append(value)

		if value == WHITESPACE:
			print(' ', end='')
		elif value == LINE_BREAK:
			print(' '*(max(DECODE_PRINT_MARGIN-len(word), 1)), end='')
			print(",".join([hex(x) for x in word]))
			word = []

		elif value in decodingMap.keys():
			print(decodingMap[value], end='')
		elif value >= CHAR_A and value <= CHAR_Z:
			print(chr(value + ord('A')), end='')
		elif value >= CHAR_a and value <= CHAR_z:
			print(chr(value - CHAR_a + ord('a')), end='')
		else:
			print(f"Unknown index: '{hexString}'", file=sys.stderr)

	print(' '*(max(DECODE_PRINT_MARGIN - 1 -len(word), 1)), end='')
	print(",".join([hex(x) for x in word]))


def getDelimiterData(text):
	startCountPairs = []
	currentStart = 0
	lineBreakCounter = 0
	for i, char in enumerate(creditsText):
	
		if char == '\n':
			startCountPairs.append((currentStart, (i - lineBreakCounter) - currentStart))
	
			lineBreakCounter += 1
			currentStart = i + 1 - lineBreakCounter
	startCountPairs.append((currentStart, (len(creditsText.rstrip('\n')) - lineBreakCounter) - currentStart))
	return startCountPairs


def getDelimiterDataMultiline(text):
	startCountPairs = []
	currentStart = 0
	lineBreakCounter = 0
	for i in range(len(text) - 1):
		char = text[i]
		nextChar = text[i + 1]

		if char == '\n' and nextChar == '\n':
			startCountPairs.append((currentStart, (i - lineBreakCounter) - currentStart + 1))

			lineBreakCounter += 1
			currentStart = i + 2 - lineBreakCounter
	startCountPairs.append((currentStart, (len(text) - lineBreakCounter) - currentStart))
	return startCountPairs


if __name__ == "__main__":

	isTitleFont = True
	isMultiline = False

	if '-multiline' in sys.argv:
		isMultiline = True
		sys.argv.remove('-multiline')
	if '-paragraphFont' in sys.argv:
		isTitleFont = False
		sys.argv.remove('-paragraphFont')

	if '-d' in sys.argv:
		sys.argv.remove('-d')
		if isMultiline:
			decodeMultiline(sys.argv[1], decodeMapTitleFont if isTitleFont else decodeMapParagraphFont)
		else:
			decode(sys.argv[1], decodeMapTitleFont if isTitleFont else decodeMapParagraphFont)
		sys.exit(0)

	inFilePath = sys.argv[1]

	base, ext = os.path.splitext(inFilePath)      # x.credits.v , .txt
	base, region = os.path.splitext(base)         # x.credits , .v

	outEncodedPath = base + '.encoded'
	outMetadataPath = base + '.metadata'

	with open(inFilePath, 'r', encoding='utf-8') as creditsTextFile:
		creditsText = creditsTextFile.read()

	if isMultiline:
		startCountPairs = getDelimiterDataMultiline(creditsText)
	else:
		startCountPairs = getDelimiterData(creditsText)

	write_if_changed(outMetadataPath, ''.join(f"{pair[0]},{pair[1]},\n" for pair in startCountPairs))

	if isMultiline:
		encodedText = encodeMultiline(creditsText, encodeMapTitleFont if isTitleFont else encodeMapParagraphFont)
	else:
		encodedText = encode(creditsText, encodeMapTitleFont if isTitleFont else encodeMapParagraphFont)

	write_if_changed(outEncodedPath, encodedText)
