#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <GL/glut.h>

/*
** �X�e���I�\���̑I��
*/
#define NONE       0
#define QUADBUF    1
#define BARRIER    2
#define STEREO     NONE

/*
** �f�B�X�v���C�T�C�Y�̑I��
*/
#define CRT17      0
#define VRROOM     1
#define DISPSIZE   CRT17

/*
** �w�b�h�g���b�L���O���邩�ǂ����iOnyx�̂݁j
*/
#define TRACKING   0

/*
** �Q�[�����[�h���g�����ǂ���
*/
#define GAMEMODE   0

/*
** �����o���A�p�̉摜�𐶐�����ۂ̃}�X�N�Ɏg��
** �X�e���V���o�b�t�@�̃r�b�g
*/
#define BARRIERBIT 1

/*
** ���ł���ʂ̍ő吔
*/
#define MAXBALL 100

/*
** ���ݒ�
*/
#define PX 0.0           /* �����ʒu�@�@�@�@�@�@�@�@�@�@ */
#define PY 1.5           /* �����ʒu�@�@�@�@�@�@�@�@�@�@ */
#define PZ (-5.0)        /* �����ʒu�@�@�@�@�@�@�@�@�@�@ */
#define TIMESCALE 0.01   /* �t���[�����Ƃ̎��ԁ@�@�@�@�@ */
#define G -9.8           /* �d�͉����x�@�@�@�@�@�@�@�@�@ */
#define SPEED 25.0       /* ���X�s�[�h�@�@�@�@�@�@�@�@�@ */
#define PI 0.3           /* �����͈́@�@�@�@�@�@�@�@�@�@ */

#define zNear 0.3        /* �O���ʂ̈ʒu�@�@�@�@�@�@�@�@ */
#define zFar 20.0        /* ����ʂ̈ʒu�@�@�@�@�@�@�@�@ */

#if DISPSIZE == VRROOM
#define W 6.0
#define H 1.5
#define D 2.0
#define P 0.06
#elif DISPSIZE == CRT17
#define W 2.0
#define H 1.5
#define D 2.0
#define P 0.06
#endif

/*
** �w�b�h�g���b�L���O�Ɏg���֐��iOnyx�̂݁j
*/
#if TRACKING
extern int startIsoTrak(char *);
extern void stopIsoTrak(int);
extern float *getIsoTrak(int);
int fd, lock = 0;
#endif

/*
** ���̃f�[�^
*/
struct balllist {
  double vx, vy, vz;     /* �����x�@�@�@�@�@�@�@�@�@�@�@ */
  double px, py, pz;     /* ���݈ʒu�@�@�@�@�@�@�@�@�@�@ */
  int c;                 /* �F�ԍ��@�@�@�@�@�@�@�@�@�@�@ */
  struct balllist *p, *n;
} start, stop, *reserve;

float bx, by, bz;        /* �w�b�h�g���b�L���O�̊�ʒu */
int cx, cy;              /* �E�B���h�E�̒��S�@�@�@�@�@�@ */
int ballcount = 0;       /* �΂�T����Ă���{�[���̐��@ */
double parallax = P;     /* �����@�@�@�@�@�@�@�@�@�@�@�@ */

/*
* �n��
*/
void myGround(double height)
{
  static GLfloat ground[][4] = {
    { 0.6f, 0.6f, 0.6f, 1.0f },
    { 0.3f, 0.3f, 0.3f, 1.0f }
  };
  
  int i, j;
  
  glBegin(GL_QUADS);
  glNormal3d(0.0, 1.0, 0.0);
  for (j = -10; j <= 10; j++) {
    for (i = -10; i < 10; i++) {
      glMaterialfv(GL_FRONT, GL_DIFFUSE, ground[(i + j) & 1]);
      glVertex3d((GLdouble)i, height, (GLdouble)j);
      glVertex3d((GLdouble)i, height, (GLdouble)(j + 1));
      glVertex3d((GLdouble)(i + 1), height, (GLdouble)(j + 1));
      glVertex3d((GLdouble)(i + 1), height, (GLdouble)j);
    }
  }
  glEnd();
}

/*
* �V�[���̕`��
*/
void scene(void)
{
  static GLfloat ballColor[][4] = {    /* ���̐F */
    { 0.2f, 0.2f, 0.2f, 1.0f },
    { 0.2f, 0.2f, 0.8f, 1.0f },
    { 0.2f, 0.8f, 0.2f, 1.0f },
    { 0.2f, 0.8f, 0.8f, 1.0f },
    { 0.8f, 0.2f, 0.2f, 1.0f },
    { 0.8f, 0.2f, 0.8f, 1.0f },
    { 0.8f, 0.8f, 0.2f, 1.0f },
    { 0.8f, 0.8f, 0.8f, 1.0f },
  };
  struct balllist *p;
  
  myGround(0.0);
  glPushMatrix();
  glMaterialfv(GL_FRONT, GL_DIFFUSE, ballColor[0]);
  glTranslated(PX, PY, PZ);
  glutSolidCube(0.5);
  glPopMatrix();
  
  for (p = start.n; p != &stop; p = p->n) {
    glPushMatrix();
    glTranslated(p->px, p->py, p->pz);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, ballColor[p->c]);
    glutSolidSphere(0.1, 16, 8);
    glPopMatrix();
  }
}

/*
 * ���̕��o
 */
int shot(int value)
{
  int count = 0;

  while (reserve && --value >= 0) {
    double u, v;
    struct balllist *p;
    
    p = reserve;
    reserve = reserve->n;
    
    u = (2.0 * (double)rand() / (double)RAND_MAX - 1.0) * PI;
    v = (2.0 * (double)rand() / (double)RAND_MAX - 1.0) * PI;
    
    p->vx = SPEED * sin(u) * cos(v);
    p->vy = SPEED *          sin(v);
    p->vz = SPEED * cos(u) * cos(v);
    
    p->px = PX;
    p->py = PY;
    p->pz = PZ;
    
    (start.n = (p->n = start.n)->p = p)->p = &start;

    ++count;
  }

  return count;
}

/*
* �V�[���̍X�V
*/
void update(void)
{
  struct balllist *p, *n;
  
  for (p = start.n; p != &stop; p = n) {
    n = p->n;
    
    p->px += p->vx * TIMESCALE;
    p->vy += G * TIMESCALE;
    p->py += p->vy * TIMESCALE;
    p->pz += p->vz * TIMESCALE;
    
    if (p->px < -5.0 || p->px > 5.0 || p->py > 5.0 || p->pz > 20.0) {
      /* �X�e�[�W�����яo�����̃f�[�^�͍폜�E��� */
      p->n->p = p->p;
      p->p->n = p->n;
      p->n = reserve;
      reserve = p;

      if (ballcount > 0) shot(1);
    }
    else if (p->py < 0.1) {
      /* �����Ȓ��˕Ԃ�v�Z */
      p->vy = -p->vy * 0.8;
    }
  }
}

/*
* ��ʕ\��
*/
void display(void)
{
  static GLfloat lightpos[] = { 3.0, 4.0, 5.0, 0.0 }; /* �����̈ʒu */
  
  GLdouble k = 0.5 * zNear / D;
  GLdouble f = parallax * k;
  GLdouble w = W * k;
  GLdouble h = H * k;
  
  float ex, ey, ez;
  
#if TRACKING
  if (lock) {
    float *handle = getIsoTrak(0);
    
    if (handle) {
      ex = (handle[0] - bx) * 0.02;
      ey = (by - handle[1]) * 0.02;
      ez = (handle[2] - bz) * 0.02;
    }
    else {
      ex = ey = ez = 0.0;
    }
  }
  else {
    ex = ey = ez = 0.0;
  }
#else
  ex = ey = ez = 0.0;
#endif
  
  /* �E�ڂ̉摜 */
#if STEREO == QUADBUF
  glDrawBuffer(GL_BACK_RIGHT);
#elif STEREO == BARRIER
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
#endif
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  
#if STEREO != NONE
  glFrustum(-w - f, w - f, -h, h, zNear, zFar);
#else
  glFrustum(-w, w, -h, h, zNear, zFar);
#endif
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslated(-parallax * 0.5, 0.0, -D);
  
  /* ���_�̈ړ��i���̂̕������Ɉڂ��j*/
  glTranslated(ex, ey - PY, ez - 10.0);
  
  /* �����̈ʒu��ݒ� */
  glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
  
#if STEREO == BARRIER
  /* ����C���ɂq�E�a��\�� */
  glStencilFunc(GL_NOTEQUAL, BARRIERBIT, BARRIERBIT);
  glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_TRUE);
  glNewList(1, GL_COMPILE_AND_EXECUTE);
#endif
  /* �V�[���̕`�� */
  scene();
#if STEREO == BARRIER
  glEndList();
  /* �������C���ɂf��\�� */
  glStencilFunc(GL_EQUAL, BARRIERBIT, BARRIERBIT);
  glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_FALSE);
  /* �V�[���̕`�� */
  glCallList(1);
#endif
  
#if STEREO != NONE
  
  /* ���ڂ̉摜 */
#  if STEREO == QUADBUF
  glDrawBuffer(GL_BACK_LEFT);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#  endif
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-w + f, w + f, -h, h, zNear, zFar);
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslated(parallax * 0.5, 0.0, -D);
  
  /* ���_�̈ړ��i���̂̕������Ɉڂ��j*/
  glTranslated(ex, ey - PY, ez - 10.0);
  
  /* �����̈ʒu��ݒ� */
  glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
  
#  if STEREO == BARRIER
  /* �������C���ɂq�E�a��\�� */
  glStencilFunc(GL_EQUAL, BARRIERBIT, BARRIERBIT);
  glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_TRUE);
  glClear(GL_DEPTH_BUFFER_BIT);
#  endif
  /* �V�[���̕`�� */
  glCallList(1);
#  if STEREO == BARRIER
  /* ����C���ɂf��\�� */
  glStencilFunc(GL_NOTEQUAL, BARRIERBIT, BARRIERBIT);
  glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_FALSE);
  /* �V�[���̕`�� */
  glCallList(1);
#  endif

#endif

  glutSwapBuffers();
  
  /* �V�[���̍X�V */
  update();
}

/*
* ��ʂ̏�����
*/
void resize(int w, int h)
{
  /* �E�B���h�E�̒��S */
  cx = w / 2;
  cy = h / 2;
  
  /* �E�B���h�E�S�̂��r���[�|�[�g�ɂ��� */
  glViewport(0, 0, w, h);
  
  /* �X�e���V���o�b�t�@�Ƀ}�X�N��`�� */
#if STEREO == BARRIER
  glClearStencil(0);
  glStencilFunc(GL_ALWAYS, BARRIERBIT, BARRIERBIT);
  glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
  glDisable(GL_DEPTH_TEST);
  glDrawBuffer(GL_NONE);
  glClear(GL_STENCIL_BUFFER_BIT);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-0.5, (GLdouble)w, -0.5, (GLdouble)h, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glBegin(GL_LINES);
  for (int x = 0; x < w; x += 2) {
    glVertex2d(x, 0);
    glVertex2d(x, h - 1);
  }
  glEnd();
  glFlush();
  glDrawBuffer(GL_BACK);
  glEnable(GL_DEPTH_TEST);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
#endif
}

void keyboard(unsigned char key, int x, int y)
{
  /* ESC �� q ���^�C�v������I�� */
  if (key == '\033' || key == 'q') {
#if TRACKING
    stopIsoTrak(fd);
#endif
#if GAMEMODE
    glutLeaveGameMode();
#endif
    exit(0);
  }
  else if (key == ' ') {
    ballcount += shot(1);
  }
  else if (key == 'o') {
    parallax += 0.05;
  }
  else if (key == 'c') {
    parallax -= 0.05;
  }
#if TRACKING
  else if (key == 's') {
    if (lock = 1 - lock) {
      float *handle = getIsoTrak(0);
      
      if (handle) {
        bx = handle[0];
        by = handle[1];
        bz = handle[2];
      }
    }
  }
#endif
}

void idle(void)
{
  glutPostRedisplay();
}

void mouse(int button, int state, int x, int y)
{
  if (state == GLUT_DOWN) {
    switch (button) {
    case GLUT_LEFT_BUTTON:
      if (reserve) {
        double u, v;
        struct balllist *p;
        
        p = reserve;
        reserve = reserve->n;
        
        u = (double)(x - cx) * PI / (double)cx;
        v = (double)(cy - y) * PI / (double)cy;
        
        p->vx = SPEED * sin(u) * cos(v);
        p->vy = SPEED *          sin(v);
        p->vz = SPEED * cos(u) * cos(v);
        
        p->px = PX;
        p->py = PY;
        p->pz = PZ;
        
        (start.n = (p->n = start.n)->p = p)->p = &start;
      }
      break;
    case GLUT_MIDDLE_BUTTON:
      glutIdleFunc(0);
      break;
    case GLUT_RIGHT_BUTTON:
      glutPostRedisplay();
      break;
    default:
      break;
    }
  }
}

void init(void)
{
  struct balllist *p;
  int n = MAXBALL;
  
  start.n = &stop;
  start.p = 0;
  stop.n = 0;
  stop.p = &start;
  reserve = p = (struct balllist *)malloc(sizeof(struct balllist) * MAXBALL);
  
  while (--n > 0) {
    p->c = n % 8;
    p->n = p + 1;
    ++p;
  }
  p->c = 7;
  p->n = 0;
  
  /* �����ݒ� */
  glClearColor(1.0, 1.0, 1.0, 0.0);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  
#if STEREO == BARRIER
  glEnable(GL_STENCIL_TEST);
#endif
}

int main(int argc, char *argv[])
{
#if TRACKING
  glutInitWindowPosition(0, 600);
  glutInitWindowSize(1920, 480);
#endif
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE
#if STEREO == QUADBUF
    | GLUT_STEREO
#elif STEREO == BARRIER
    | GLUT_STENCIL
#endif
    );
#if GAMEMODE
  glutGameModeString("width=1024 height=768 bpp~24 hertz=100");
  glutEnterGameMode();
#else
  glutCreateWindow(argv[0]);
#  if STEREO == BARRIER
  glutFullScreen();
#  endif
#endif
  glutDisplayFunc(display);
  glutReshapeFunc(resize);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse);
  glutIdleFunc(idle);
  init();
#if TRACKING
  fd = startIsoTrak("/dev/ttyd5");
#endif
  glutMainLoop();
  return 0;
}
