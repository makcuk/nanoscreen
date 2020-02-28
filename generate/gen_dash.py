#!/usr/bin/env python3

from PIL import ImageFile, Image, ImageDraw, ImageFont
from time import localtime, strftime

fp = open("dash_template.bmp", "rb")

p = ImageFile.Parser()

while 1:
    s = fp.read()
    if not s:
        break
    p.feed(s)

im = p.close()

fnt = ImageFont.truetype('Arial.ttf', 25)
fnt_s = ImageFont.load('spleen-5x8.pil')
fnt_b = ImageFont.load("spleen-16x32.pil")
d = ImageDraw.Draw(im)
c_time = strftime("%H:%M", localtime())
c_date = strftime("%d %b", localtime())
c_day = strftime("%a", localtime())
d.text((10, 12), c_time, font=fnt_b, fill=(246,11,11,255))
d.text((95, 7), c_date, font=fnt_s, fill=(246,11,11))
d.text((95, 21), c_day, font=fnt_s, fill=(246,11,11))
d.text((95, 35), strftime("%Y", localtime()), font=fnt_s, fill=(246,11,11))
#d.text((30, 55), "15:09", font=fnt_b, fill=(246,11,11))
im.save('dash.bmp')

