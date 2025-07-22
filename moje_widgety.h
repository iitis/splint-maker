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
	QPushButton* btLiczOklu;
	QDoubleSpinBox* okluDist;
	QGroupBox* rodzajDanych;
	QRadioButton* zPomiaru;
	QRadioButton* zObliczen;

	WidgetWyborSiatek(QWidget* p = nullptr) : QWidget(p) {
		infoLabel00 = new QLabel(QString::fromUtf8("Aby wygenerować szynę, potrzebujesz siatkę reprezentującą szczękę i siatkę reprezentującą powierzchnię okluzyjną przesuniętą do pozycji terapeutycznej."));
		infoLabel00->setWordWrap(true);
		infoLabel01 = new QLabel(QString::fromUtf8("Jeśli nie masz powierzchni okluzyjnej, możesz użyć siatki reprezentującej żuchwę i spróbować wygenerować tę powierzchnię."));
		infoLabel01->setWordWrap(true);
		infoLabel = new QLabel(QString::fromUtf8("Możesz wczytać wcześniej przygotowany plik .atmdl. Siatki zostaną rozpoznane w oparciu o słowa kluczowe lub etykiety."));
		infoLabel->setWordWrap(true);
		infoLabel2 = new QLabel(QString::fromUtf8("Jeśli siatki nie zostaną poprawnie rozpoznane, albo wolisz wczytać je w standardowy sposób, możesz wskazać ich funkcję korzystając z przycisków poniżej i klikając właściwą siatkę w oknie projektu."));
		infoLabel2->setWordWrap(true);

		rodzajDanych = new QGroupBox(QString::fromUtf8("Model żuchwy jest oryginalnie w pozycji:"));
		zPomiaru = new QRadioButton(QString::fromUtf8("terapeutycznej"));
		zPomiaru->setChecked(true);
		zObliczen = new QRadioButton(QString::fromUtf8("zerowej"));
		QHBoxLayout* hl = new QHBoxLayout();
		hl->addWidget(zObliczen);
		hl->addWidget(zPomiaru);

		rodzajDanych->setLayout(hl);

		btSzczLabel = new QLabel(QString::fromUtf8("wybierz siatkę szczęki:"));
		btSzczLabel->setStyleSheet(QString::fromUtf8("color:#f00;"));

		btZuchLabel = new QLabel(QString::fromUtf8("wybierz siatkę zuchwy:"));
		
		btOkluLabel = new QLabel(QString::fromUtf8("wybierz siatkę okluzji:"));
		btOkluLabel->setStyleSheet(QString::fromUtf8("color:#f00;"));

		okluDistLabel = new QLabel(QString::fromUtf8("określ odległość dla okluzji:"));
		
		btAtmdl = new QPushButton(QString::fromUtf8("wczytaj przygotowany plik .atmdl"));
		btSzcz = new QPushButton("...");
		btZuch = new QPushButton("...");
		btOklu = new QPushButton("...");
		btLiczOklu = new QPushButton(QString::fromUtf8("wygeneruj siatkę okluzji"));
		okluDist = new QDoubleSpinBox();
		okluDist->setValue(2.0);

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

		enableLiczOklu(false);

		this->setLayout(l);
	}

	void enableLiczOklu(bool b) {
		okluDistLabel->setEnabled(b);
		okluDist->setEnabled(b);
		btLiczOklu->setEnabled(b);
	}

	void ustawSzczeke(QString label) {
		btSzczLabel->setText(label);
		btSzczLabel->setStyleSheet(QString::fromUtf8("color:#00f;"));

		btSzcz->setText(QString::fromUtf8("zmień siatkę szczęki..."));
	}

	void ustawOkluzje(QString label) {
		btOkluLabel->setText(label);
		btOkluLabel->setStyleSheet(QString::fromUtf8("color:#00f;"));

		btOklu->setText(QString::fromUtf8("zmień siatkę okluzji..."));
	}
	void ustawZuchwe(QString label) {
		btZuchLabel->setText(label);
		btZuchLabel->setStyleSheet(QString::fromUtf8("color:#00f;"));

		btZuch->setText(QString::fromUtf8("zmień siatkę żuchwy..."));
	}
};

class WidgetEtap1 : public QWidget {
	Q_OBJECT
public:
	QLabel* meshDividerLabel;
	QSpinBox* meshDivider;
	QPushButton* btStart;

	WidgetEtap1(QWidget* p = nullptr) : QWidget(p) {
		meshDividerLabel = new QLabel(QString::fromUtf8("określ gęstość siatki [lb.węzłów/1 mm]"));
		meshDivider = new QSpinBox();
		meshDivider->setValue(10);
		btStart = new QPushButton(QString::fromUtf8("Dalej"));

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
		infoLabel = new QLabel(QString::fromUtf8("Opcjonalnie, teraz możesz przyciąć siatkę szczęki aby np. usunąć podniebienie."));
		infoLabel->setWordWrap(true);
		insideDistLabel = new QLabel(QString::fromUtf8("Odległość w mm"));
		insideDist = new QDoubleSpinBox();
		insideDist->setValue(10.0);
		btStart = new QPushButton(QString::fromUtf8("przytnij do okluzji"));

		QFormLayout* l = new QFormLayout(this);

		l->addRow(infoLabel);
		l->addRow(insideDistLabel, insideDist);
		l->addRow(btStart);

		this->setLayout(l);
	}
};


class WidgetEtap12 : public QWidget {
	Q_OBJECT
public:
	QLabel* insideDistLabel;
	QDoubleSpinBox* insideDist;
	QPushButton* btStart;

	WidgetEtap12(QWidget* p = nullptr) : QWidget(p) {
		insideDistLabel = new QLabel(QString::fromUtf8("rozepchanie wnetrza w mm"));
		insideDist = new QDoubleSpinBox();
		insideDist->setValue(0.3);
		btStart = new QPushButton(QString::fromUtf8("generuj wnętrze"));

		QFormLayout* l = new QFormLayout(this);

		l->addRow(insideDistLabel, insideDist);
		l->addRow(btStart);

		this->setLayout(l);
	}
};


class WidgetEtap13 : public QWidget {
	Q_OBJECT
public:
	QLabel* outsideDistLabel;
	QDoubleSpinBox* outsideDist;
	QPushButton* btStart, *btStart2, *btStart3;

	WidgetEtap13(QWidget* p = nullptr) : QWidget(p) {
		outsideDistLabel = new QLabel(QString::fromUtf8("rozepchanie wierzchu w mm"));
		outsideDist = new QDoubleSpinBox();
		outsideDist->setValue(1.3);
		btStart2 = new QPushButton(QString::fromUtf8("generuj wierzch"));
		
		QFormLayout* l = new QFormLayout(this);

		l->addRow(outsideDistLabel, outsideDist);
		l->addRow(btStart2);
		

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

		btStart = new QPushButton(QString::fromUtf8("Dalej"));

		l->addRow(btStart);

		this->setLayout(l);
	}
};

class MojWidget : public QWidget {
	Q_OBJECT
public:
	WidgetWyborSiatek* wybor_siatek;
	WidgetPrzytnijSzczeke* przytnij_szczene;
	WidgetEtap1* etap11;
	WidgetEtap12* etap12;
	WidgetEtap13* etap13;
	WidgetInfo *widget_info, *etap14, *etap15, *info_koncowe, *info_zapis, *info_exchange;
	QPushButton* dotnij_btn;

	MojWidget(QWidget* p = nullptr) : QWidget(p) {
		QFormLayout* l = new QFormLayout(this);
		l->setContentsMargins(0, 0, 0, 0);
		l->setSpacing(0);



		this->setLayout(l);
	}
};