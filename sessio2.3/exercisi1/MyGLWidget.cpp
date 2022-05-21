#include "MyGLWidget.h"

#include <iostream>

MyGLWidget::MyGLWidget (QWidget* parent) : QOpenGLWidget(parent), program(NULL)
{
	setFocusPolicy(Qt::StrongFocus);  // per rebre events de teclat
	scale = 1.0f;
}

MyGLWidget::~MyGLWidget ()
{
	if (program != NULL) delete program;
}

void MyGLWidget::initializeGL ()
{
	// Cal inicialitzar l'ús de les funcions d'OpenGL
	initializeOpenGLFunctions();  

	glClearColor(0.5, 0.7, 1.0, 1.0); // defineix color de fons (d'esborrat)
	ini_camera();
	ini_camera_3persona();
	ini_camera_ortogonal();
	glEnable(GL_DEPTH_TEST);
	carregaShaders();
	creaBuffers();
}

void MyGLWidget::paintGL () 
{
	// Aquest codi és necessari únicament per a MACs amb pantalla retina.
#ifdef __APPLE__
	GLint vp[4];
	glGetIntegerv (GL_VIEWPORT, vp);
	ample = vp[2];
	alt = vp[3];
#endif

	// En cas de voler canviar els paràmetres del viewport, descomenteu la crida següent i
	// useu els paràmetres que considereu (els que hi ha són els de per defecte)
	glViewport (0, 0, ample, alt);
  
	// Esborrem el frame-buffer
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Carreguem la transformació de model
	modelTransformTerra ();

	// Activem el VAO per a pintar la caseta 
	glBindVertexArray (VAO_Casa);

	// pintem
	glDrawArrays(GL_TRIANGLES, 0, 6);

	projectTransform();
	viewTransform();

	// Carreguem la transformació de model
	modelTransform ();

	// Activem el VAO per a pintar la caseta
	glBindVertexArray (VAO_model);

	// pintem
	glDrawArrays(GL_TRIANGLES, 0, m.faces().size() * 3);

	glBindVertexArray (0);
}

void MyGLWidget::modelTransformTerra ()
{
	// Matriu de transformació de model
	glm::mat4 transform (1.0f);
	transform = glm::translate(transform, glm::vec3(0.0,-2.0,0.0));
	glUniformMatrix4fv(transLoc, 1, GL_FALSE, &transform[0][0]);
}

void MyGLWidget::modelTransform () 
{
	// Matriu de transformació de model
	glm::mat4 transform (1.0f);
	transform = glm::translate(transform, glm::vec3(9.5,-5.5,0));
	/*transform = glm::scale(transform, glm::vec3(scale));*/
	transform = glm::rotate(transform, (float)(rad*M_PI/4), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(transLoc, 1, GL_FALSE, &transform[0][0]);
}

void MyGLWidget::projectTransform() {
    glm::mat4 Proj;
	if(not ortogonal) Proj = glm::perspective(FOV, ra, znear, zfar);
    else Proj = glm::ortho(left, right, bottom, top, znear, zfar);
    glUniformMatrix4fv (projLoc, 1, GL_FALSE, &Proj[0][0]);
}

void MyGLWidget::viewTransform() {
    glm::mat4 View(1.0f);/* = glm::lookAt ( OBS, centrat, up); //UP*/
	View = glm::translate(View, glm::vec3(0.0f,centrat.y,-(diametre+radi)));
	//VM = VM*Rotate(-phi,0,0,1);
    //View = glm::translate(View, glm::vec3(0.0f,0.0f,dist));
    View = glm::rotate(View, glm::radians(theta), glm::vec3(0.0f, 1.0f, 0.0f));
    View = glm::rotate(View, glm::radians(-psi), glm::vec3(1.0f, 0.0f, 0.0f));
    View = glm::translate(View,centrat);
	glUniformMatrix4fv (viewLoc, 1, GL_FALSE, &View[0][0]);
}

void MyGLWidget::resizeGL (int w, int h) 
{
	ample = w;
	alt = h;
	//Manetnir el aspect ratio del WidgetGL 
	float ra_v = float(w) / float(h);
	ra = ra_v;
	//Comprovem que la camera no estigui e perspectiva ortogonal
	if(not ortogonal) {
		/*
		  En el cas que el ra sigui inferior(que l'usuari esta deformant la finestra o que no sigui quadrada)
		  FOV = 2*arcsin(R/d)
		*/
		if(ra < 1) FOV = 2*glm::atan(glm::tan(glm::asin(rad/diametre))/ra);
		/*
		  Si el widget GL es quadrat -> FOV = M_PI/2.0
		*/
		else FOV = M_PI/2.0;
	} else {
		//Si tenim la camera en perpectiva ortogonal
		if(ra > 1) {
			bottom = left = -(float)(ra*(2*radi));
			top = right = (float) (ra*(2*radi));
		}
	}
}

void MyGLWidget::mousePressEvent(QMouseEvent *e) {
    xClick = e->x();
    yClick = e->y();
    if (e->button() & Qt::LeftButton &&! (e->modifiers() & (Qt::ShiftModifier|Qt::AltModifier|Qt::ControlModifier))) DoingInteractive = ROTATE;
}

void MyGLWidget::mouseReleaseEvent(QMouseEvent *) {
    DoingInteractive = NONE;
}

void MyGLWidget::mouseMoveEvent(QMouseEvent *e) {
    makeCurrent();
    if (DoingInteractive == ROTATE) {
		// Aquí cal completar per fer la rotació...
		theta += e->x() - xClick;
		psi -= e->y() - yClick;
		viewTransform ();
    }
    xClick = e->x();
    yClick = e->y();
    update ();
}

void MyGLWidget::keyPressEvent(QKeyEvent* event) 
{
	makeCurrent();
	switch (event->key()) {
        case Qt::Key_S: { // escalar a més gran
			scale += 0.05;
			break;
        }
        case Qt::Key_D: { // escalar a més petit
			scale -= 0.05;
			break;
        }
        case Qt::Key_R: {
			rad += 0.5;
			break;
        }
        case Qt::Key_O: {
			if(not ortogonal) ortogonal = true;
			else ortogonal = false;
			break;
        }
	    case Qt::Key_Z: {
			FOV = glm::radians(40.0);
			break;
        }
        case Qt::Key_X: {
			FOV = M_PI/2.0;
			break;
        }
        default: event->ignore(); break;
 	}
	update();
}

void MyGLWidget::creaBuffers () 
{
	// Dades de la caseta
	// Dos VBOs, un amb posició i l'altre amb color
	glm::vec3 posicio[6] = {
		glm::vec3(-2.5, 0.0, -2.5),
		glm::vec3(-2.5, 0.0, 2.5),
		glm::vec3( 2.5, 0.0, -2.5),
		glm::vec3( 2.5, 0.0, -2.5),
		glm::vec3( 2.5, 0.0, 2.5),
		glm::vec3(-2.5, 0.0, 2.5)
	};
	glm::vec3 color[6] = {
		glm::vec3(1, 0, 0),
		glm::vec3(0, 1, 0),
		glm::vec3(0, 0, 1),
		glm::vec3(0, 0, 1),
		glm::vec3(0, 1, 0),
		glm::vec3(1, 0, 0)
	};

	// Creació del Vertex Array Object per pintar
	glGenVertexArrays(1, &VAO_Casa);
	glBindVertexArray(VAO_Casa);

	GLuint VBO_Casa[2];
	glGenBuffers(2, VBO_Casa);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_Casa[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(posicio), posicio, GL_STATIC_DRAW);

	// Activem l'atribut vertexLoc
	glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vertexLoc);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_Casa[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(color), color, GL_STATIC_DRAW);

	// Activem l'atribut colorLoc
	glVertexAttribPointer(colorLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(colorLoc);

	m.load("../figures3D/Patricio.obj");

	// Creació del Vertex Array Object per pintar
	glGenVertexArrays(1, &VAO_model);
	glBindVertexArray(VAO_model);

	GLuint VBO_model[2];
	glGenBuffers(2, VBO_model);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_model[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * m.faces().size() * 3 * 3, m.VBO_vertices(), GL_STATIC_DRAW);

	// Activem l'atribut vertexLoc
	glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vertexLoc);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_model[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * m.faces().size() * 3 * 3, m.VBO_matdiff(), GL_STATIC_DRAW);
	// Activem l'atribut colorLoc
	glVertexAttribPointer(colorLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(colorLoc);
	glBindVertexArray (0);
}

void MyGLWidget::ini_camera() {
    FOV = M_PI/2.0;
    ra = 1.0f;
    znear = 0.4f;
    zfar = 3.0f;
    OBS = glm::vec3(0,0,0);
    VRP = glm::vec3(0,2,0);
    up = glm::vec3(0,1,0);
}

void MyGLWidget::ini_camera_3persona() {
    min = glm::vec3(-2.5,-4,-2.5);
    max = glm::vec3(2.5,0,2.5);
    ra = 1.0f;
    centrat = glm::vec3((max.x+min.x)/2,(max.y+min.y)/2,(max.z+min.z)/2);
    radi = glm::distance(min,max)/2;
    diametre = radi*2;
    //alfa = glm::asin(radi/diametre);
    //std::cout << alfa << std::endl;
    FOV = M_PI/2.0;
    OBS = centrat+((diametre)*glm::vec3(0,0,1));
    znear = radi;
    zfar = diametre+radi;
}

void MyGLWidget::ini_camera_ortogonal() {
    min = glm::vec3(-2.5,0,-2.5);
    max = glm::vec3(2.5,4,2.5);
    bottom = left = -(ra*(2*radi));
    right = top = (ra*(2*radi));
    ra = (float)(right-left)/(float)(top-bottom);
    centrat = glm::vec3((max.x+min.x)/2,(max.y+min.y)/2,(max.z+min.z)/2);
    std::cout << centrat.x << ","<< centrat.y << "," << centrat.z << std::endl;
    radi = glm::distance(min,max)/2;
    diametre = radi*2;
    //alfa = glm::asin(radi/diametre);
    //std::cout << alfa << std::endl;
    OBS = centrat+((diametre)*glm::vec3(0,0,1));
    std::cout << OBS.x << ","<< OBS.y << "," << OBS.z << std::endl;
    znear = radi;
    zfar = diametre+radi;
}

void MyGLWidget::carregaShaders()
{
	// Creem els shaders per al fragment shader i el vertex shader
	QOpenGLShader fs (QOpenGLShader::Fragment, this);
	QOpenGLShader vs (QOpenGLShader::Vertex, this);
	// Carreguem el codi dels fitxers i els compilem
	fs.compileSourceFile("shaders/basicShader.frag");
	vs.compileSourceFile("shaders/basicShader.vert");
	// Creem el program
	program = new QOpenGLShaderProgram(this);
	// Li afegim els shaders corresponents
	program->addShader(&fs);
	program->addShader(&vs);
	// Linkem el program
	program->link();
	// Indiquem que aquest és el program que volem usar
	program->bind();

	// Obtenim identificador per a l'atribut “vertex” del vertex shader
	vertexLoc = glGetAttribLocation (program->programId(), "vertex");
	// Obtenim identificador per a l'atribut “color” del vertex shader
	colorLoc = glGetAttribLocation (program->programId(), "color");
	// Uniform locations
	transLoc = glGetUniformLocation(program->programId(), "TG");
	projLoc = glGetUniformLocation(program->programId(), "PM");
	viewLoc = glGetUniformLocation (program->programId(), "VM");
}

