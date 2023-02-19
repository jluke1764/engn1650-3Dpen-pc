'''
code for reading image and outputting 3d position
Based on algorithms in ball2pos_simp.kc
'''


import cv2
import math

xres = 1024
yres = 768

class Point3D:
    def __init__(self, x, y, z):
        self.x = x
        self.y = y
        self.z = z
       
        
def sph2conic(cx,cy,cz):
    cx2 = cx*cx; cy2 = cy*cy; cz2 = cz*cz;
    a = 1-cy2-cz2; e = cy*cz*2;
    c = 1-cx2-cz2; d = cx*cz*2;
    f = 1-cx2-cy2; b = cx*cy*2;

    e = e*ihaf.z - c*ihaf.y*2 - b*ihaf.x;
    d = d*ihaf.z - a*ihaf.x*2 - b*ihaf.y;
    f = f*ihaf.z*ihaf.z - a*ihaf.x*ihaf.x - b*ihaf.x*ihaf.y - c*ihaf.y*ihaf.y - d*ihaf.x - e*ihaf.y;
    
    return a,b,c,d,e,f 

class linefit3d:
    def __init__(self):
        self.s = 0
        self.sx = 0
        self.sy = 0
        self.sz = 0
        self.sxx = 0
        self.syy = 0
        self.szz = 0
        self.sxy = 0
        self.sxz = 0
        self.syz = 0
        
    def addpoint (self, x, y, z):
        self.s += 1
        self.sx += x
        self.sy += y
        self.sz += z
        self.sxx += x*x
        self.syy += y*y
        self.szz += z*z
        self.sxy += x*y
        self.sxz += x*z
        self.syz += y*z
    
    def getplane (self):
        r = 1/self.s
        cent = Point3D(self.sx*r,self.sy*r,self.sz*r)
        sxx = self.sxx - self.sx*cent.x
        sxy = self.sxy - self.sx*cent.y
        syy = self.syy - self.sy*cent.y
        sxz = self.sxz - self.sx*cent.z
        szz = self.szz - self.sz*cent.z
        syz = self.syz - self.sy*cent.z
        norm = Point3D(sxy*syz - sxz*syy, sxy*sxz - sxx*syz, sxx*syy - sxy*sxy)
        return cent, norm


estpos = Point3D(10, 10, 10)
ihaf = Point3D(xres/2, yres/2, yres/2 * 1.96) # 1.96 is cot(27deg) for my laptop's camera 54 deg fov.
img = cv2.imread('./test2.png')

print(img.shape)

lf = linefit3d()
nnorm = Point3D(0,0,0)
flag = False

for y in range(0, yres, 10):
    wasBlack = True
    for x in range(0, xres, 2):
        if wasBlack and img[y,x,0] > 150: # temp threshold for white
            wasBlack = False
            
            cv2.circle(img, (x,y), 4, (0,255,0), 2)
            vx = x-ihaf.x
            vy = y-ihaf.y
            vz = ihaf.z
            t = 1/math.sqrt(math.pow(vx,2) + math.pow(vy,2) + math.pow(vz,2));
            lf.addpoint(vx*t,vy*t,vz*t);

cent, norm = lf.getplane()

t = cent.x*norm.x + cent.y*norm.y + cent.z*norm.z;
t = 1/math.sqrt(norm.x*norm.x + norm.y*norm.y + norm.z*norm.z - t*t);
estpos.x = norm.x*t
estpos.y = norm.y*t
estpos.z = norm.z*t 

# converting it for screen 
t = ihaf.z/estpos.z;
sx = estpos.x*t + ihaf.x
sy = estpos.y*t + ihaf.y
cv2.circle(img, (int(sx),int(sy)), 4, (255,0,0), 2)

print(str(estpos.x) + ',' + str(estpos.y) + ',' + str(estpos.z))
print(str(sx) + ',' + str(sy) )

cv2.imshow("image", img)
cv2.waitKey(0)