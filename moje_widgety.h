#pragma once

#include <QtWidgets>


class WidgetWyborSiatek : public QWidget {
	Q_OBJECT
public:
	QLabel* infoLabel00;
	QLabel* infoLabel01;
	QLabel* infoLabel;
	QLabel* infoLabel2;
	QLabel* btSzczLabel;
	QLabel* btZuchLabel;
	QLabel* btOkluLabel;
	QLabel* okluDistLabel;
	QPushButton* btAtmdl;
	QPushButton* btSzcz;
	QPushButton* btZuch;
	QPushButton* btOklu;
	QPushButton* btLiczOklu, *btLiczOkluAg;
	QDoubleSpinBox* okluDist;
	QGroupBox* rodzajDanych;
	QRadioButton* zPomiaru;
	QRadioButton* zObliczen;

	WidgetWyborSiatek(QWidget* p = nullptr) : QWidget(p) {
		infoLabel00 = new QLabel(QString::fromUtf8("To generate a splint, you need a mesh representing the upper jaw and a mesh representing the occlusal surface shifted to the therapeutic position."));
		infoLabel00->setWordWrap(true);
		infoLabel01 = new QLabel(QString::fromUtf8("If you do not have an occlusal surface, you can use a mesh representing the lower jaw and try to generate this surface."));
		infoLabel01->setWordWrap(true);
		infoLabel = new QLabel(QString::fromUtf8("You can load a previously prepared .atmdl file. Meshes will be recognized based on keywords or labels."));
		infoLabel->setWordWrap(true);
		infoLabel2 = new QLabel(QString::fromUtf8("If the meshes are not recognized correctly, or you prefer to load them in the standard way, you can assign their function using the buttons below and by clicking the appropriate mesh in the project window."));
		infoLabel2->setWordWrap(true);

		rodzajDanych = new QGroupBox(QString::fromUtf8("The lower jaw model is originally in position:"));
		zPomiaru = new QRadioButton(QString::fromUtf8("therapeutic"));
		zObliczen = new QRadioButton(QString::fromUtf8("zero"));
		//zPomiaru->setChecked(true);
		zObliczen->setChecked(true);
		QHBoxLayout* hl = new QHBoxLayout();
		hl->addWidget(zObliczen);
		hl->addWidget(zPomiaru);

		rodzajDanych->setLayout(hl);

		btSzczLabel = new QLabel(QString::fromUtf8("select upper jaw mesh:"));
		btSzczLabel->setStyleSheet(QString::fromUtf8("color:#f00;"));

		btZuchLabel = new QLabel(QString::fromUtf8("select lower jaw mesh:"));
		
		btOkluLabel = new QLabel(QString::fromUtf8("select occlusion mesh:"));
		btOkluLabel->setStyleSheet(QString::fromUtf8("color:#f00;"));

		okluDistLabel = new QLabel(QString::fromUtf8("set occlusion distance:"));
		
		btAtmdl = new QPushButton(QString::fromUtf8("load prepared .atmdl file"));
		btSzcz = new QPushButton("...");
		btZuch = new QPushButton("...");
		btOklu = new QPushButton("...");
		btLiczOklu = new QPushButton(QString::fromUtf8("generate occlusion mesh"));
		btLiczOkluAg = new QPushButton(QString::fromUtf8("generate occlusion mesh (test)"));
		okluDist = new QDoubleSpinBox();
		okluDist->setValue(5.0);

		QFormLayout* l = new QFormLayout(this);
		l->addRow(infoLabel00);
		l->addRow(infoLabel);
		l->addRow(btAtmdl);
		l->addRow(infoLabel2);
		

		l->addRow(btSzczLabel, btSzcz);
		l->addRow(btOkluLabel, btOklu);

		l->addRow(infoLabel01);

		l->addRow(btZuchLabel, btZuch);

		l->addRow(rodzajDanych);

		l->addRow(okluDistLabel, okluDist);

		l->addRow(btLiczOklu);
		l->addRow(btLiczOkluAg);

		enableLiczOklu(false);

		this->setLayout(l);
	}

	void enableLiczOklu(bool b) {
		okluDistLabel->setEnabled(b);
		okluDist->setEnabled(b);
		btLiczOklu->setEnabled(b);
		btLiczOkluAg->setEnabled(b);
	}

	void ustawSzczeke(QString label) {
		btSzczLabel->setText(label);
		btSzczLabel->setStyleSheet(QString::fromUtf8("color:#00f;"));

		btSzcz->setText(QString::fromUtf8("change upper jaw mesh..."));
	}

	void ustawOkluzje(QString label) {
		btOkluLabel->setText(label);
		btOkluLabel->setStyleSheet(QString::fromUtf8("color:#00f;"));

		btOklu->setText(QString::fromUtf8("change occlusion mesh..."));
	}
	void ustawZuchwe(QString label) {
		btZuchLabel->setText(label);
		btZuchLabel->setStyleSheet(QString::fromUtf8("color:#00f;"));

		btZuch->setText(QString::fromUtf8("change lower jaw mesh..."));
	}
};

class WidgetGestoscSiatki : public QWidget {
	Q_OBJECT
public:
	QLabel* meshDividerLabel;
	QSpinBox* meshDivider;
	QPushButton* btStart;

	WidgetGestoscSiatki(QWidget* p = nullptr) : QWidget(p) {
		meshDividerLabel = new QLabel(QString::fromUtf8("set mesh density [nodes/1 mm]"));
		meshDivider = new QSpinBox();
		meshDivider->setValue(10);
		btStart = new QPushButton(QString::fromUtf8("Next"));

		QFormLayout* l = new QFormLayout(this);

		l->addRow(meshDividerLabel, meshDivider);
		l->addRow(btStart);

		this->setLayout(l);
	}
};


class WidgetPrzytnijSzczeke : public QWidget {
	Q_OBJECT
public:
	QLabel* infoLabel;
	QLabel* insideDistLabel;
	QDoubleSpinBox* insideDist;
	QPushButton* btStart;

	WidgetPrzytnijSzczeke(QWidget* p = nullptr) : QWidget(p) {
		infoLabel = new QLabel(QString::fromUtf8("Optionally, you can now trim the upper jaw mesh, e.g. to remove the palate."));
		infoLabel->setWordWrap(true);
		insideDistLabel = new QLabel(QString::fromUtf8("Distance in mm"));
		insideDist = new QDoubleSpinBox();
		insideDist->setValue(10.0);
		btStart = new QPushButton(QString::fromUtf8("trim to occlusion"));

		QFormLayout* l = new QFormLayout(this);

		l->addRow(infoLabel);
		l->addRow(insideDistLabel, insideDist);
		l->addRow(btStart);

		this->setLayout(l);
	}
};


class WidgetEtap123 : public QWidget {
	Q_OBJECT
public:
	QLabel* insideDistLabel;
	QDoubleSpinBox* insideDist;
	QLabel* outsideDistLabel;
	QDoubleSpinBox* outsideDist;
	QPushButton* btStart23;

	WidgetEtap123(QWidget* p = nullptr) : QWidget(p) {
		insideDistLabel = new QLabel(QString::fromUtf8("inner surface displacement [mm]"));
		outsideDistLabel = new QLabel(QString::fromUtf8("outer surface displacement [mm]"));
		insideDist = new QDoubleSpinBox();
		insideDist->setValue(0.3);
		outsideDist = new QDoubleSpinBox();
		outsideDist->setValue(1.3);
		btStart23 = new QPushButton(QString::fromUtf8("CREATE SPLINT"));

		QFormLayout* l = new QFormLayout(this);

		l->addRow(insideDistLabel, insideDist);
		l->addRow(outsideDistLabel, outsideDist);
		l->addRow(btStart23);

		this->setLayout(l);
	}
};



class WidgetInfo : public QWidget {
	Q_OBJECT
public:
	QList<QLabel*> labels;
	QPushButton* btStart;

	WidgetInfo(QStringList infos, QWidget * p = nullptr) : QWidget(p) {
		QFormLayout* l = new QFormLayout(this);

		for (auto info : infos)
		{
			QLabel* label = new QLabel(info);
			label->setWordWrap(true);
			labels.append( label );
			l->addRow(label);
		}

		btStart = new QPushButton(QString::fromUtf8("Next"));

		l->addRow(btStart);

		this->setLayout(l);
	}
};

class MojWidget : public QWidget {
	Q_OBJECT
public:
	WidgetWyborSiatek* wybor_siatek;
	WidgetPrzytnijSzczeke* przytnij_szczene;
	WidgetGestoscSiatki* etap11;
	WidgetEtap123* etap123;
	WidgetInfo *widget_info;
	WidgetInfo *info_koncowe;
	WidgetInfo *info_zapis;
	QPushButton* dotnij_btn;

	MojWidget(QWidget* p = nullptr) : QWidget(p) {
		QFormLayout* l = new QFormLayout(this);
		l->setContentsMargins(0, 0, 0, 0);
		l->setSpacing(0);



		this->setLayout(l);
	}
};