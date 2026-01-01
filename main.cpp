#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QGroupBox>
#include <QFile>
#include <QDataStream>
#include <QDir>
#include <QDateTime>
#include <QDialog>
#include <QTextEdit>
#include <QStyleFactory>

// --- DATA STRUCTURES ---
struct Medicine {
    int id;
    QString name;
    double price;
    int stock;
    QString expiry;
    QString company;

    friend QDataStream &operator<<(QDataStream &out, const Medicine &m) {
        return out << m.id << m.name << m.price << m.stock << m.expiry << m.company;
    }
    friend QDataStream &operator>>(QDataStream &in, Medicine &m) {
        return in >> m.id >> m.name >> m.price >> m.stock >> m.expiry >> m.company;
    }
};

struct CartItem {
    int medId;
    QString name;
    double price;
    int qty;
};

class MedicalStore : public QWidget {
    Q_OBJECT

private:
    QVector<Medicine> medicineList;
    QVector<CartItem> cartList;
    const QString FILE_NAME = "medicines.dat";
    const QString BACKUP_DIR = "backups";

    QLineEdit *txtId, *txtName, *txtPrice, *txtStock, *txtExpiry, *txtCompany;
    QLineEdit *txtSearch;
    QTableWidget *tableMedicines, *tableCart;
    QLabel *lblTotal;

    // --- COLORS ---
    QString primaryColor = "#0066CC"; // Medical Blue
    QString successColor = "#28A745"; // Green
    QString dangerColor  = "#DC3545"; // Red
    QString warningColor = "#FFC107"; // Orange

    // --- STYLESHEET (Fixed Ghost Buttons) ---
    QString appStyle =
        "QWidget { font-family: 'Segoe UI', sans-serif; font-size: 14px; }"
        "QWidget#CentralWidget { background-color: #F0F4F8; }"

        // Input Fields
        "QLineEdit { color: #333333; background-color: white; border: 1px solid #BDC3C7; padding: 6px; border-radius: 4px; }"
        "QLineEdit:focus { border: 2px solid #0066CC; }"

        // Tables
        "QTableWidget { background-color: white; color: #333333; gridline-color: #E0E0E0; border: 1px solid #BDC3C7; }"
        "QHeaderView::section { background-color: #E8E8E8; color: #333333; padding: 6px; border: none; font-weight: bold; }"
        "QTableWidget::item:selected { background-color: #0066CC; color: white; }"

        // Popups (Message Box / Input Dialog)
        "QDialog, QMessageBox, QInputDialog { background-color: white; color: #333333; }"
        "QDialog QLabel { color: #333333; font-weight: bold; }"

        // *** CRITICAL FIX: Force Background Color on Standard Popup Buttons ***
        "QDialog QPushButton { background-color: #0066CC; color: white; padding: 6px 15px; border-radius: 4px; min-width: 60px; }"
        "QDialog QPushButton:hover { background-color: #0055AA; }"

        // Main App Buttons (Base Style)
        "QPushButton { color: white; font-weight: bold; padding: 8px 16px; border-radius: 4px; border: none; }"
        "QPushButton:hover { margin-top: -1px; margin-bottom: 1px; }";

public:
    MedicalStore(QWidget *parent = nullptr) : QWidget(parent) {
        setObjectName("CentralWidget");
        setWindowTitle("Medical Store Management System - Professional Edition");
        resize(1300, 800);
        setStyleSheet(appStyle);

        // 1. SECURE LOGIN
        bool ok;
        QString text = QInputDialog::getText(nullptr, "Security Check", "Enter Admin Password:", QLineEdit::Password, "", &ok);
        if (!ok || text != "admin123") {
            QMessageBox::critical(nullptr, "Access Denied", "Incorrect Password.");
            exit(0);
        }

        loadData();
        performAutoBackup();

        // --- LAYOUT SETUP ---
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);

        // --- HEADER ---
        QWidget *header = new QWidget();
        header->setStyleSheet("background-color: " + primaryColor + ";");
        header->setFixedHeight(80);
        QHBoxLayout *headerLayout = new QHBoxLayout(header);

        QLabel *title = new QLabel("Medical Store System");
        title->setStyleSheet("color: white; font-size: 28px; font-weight: bold; background: transparent; padding-left: 10px;");

        QPushButton *btnBackup = new QPushButton("Backup Data");
        btnBackup->setStyleSheet("background-color: white; color: " + primaryColor + "; font-weight: bold;");
        connect(btnBackup, &QPushButton::clicked, this, [=]() { createBackup("Manual"); });

        headerLayout->addWidget(title);
        headerLayout->addStretch();
        headerLayout->addWidget(btnBackup);
        headerLayout->setContentsMargins(20, 0, 20, 0);
        mainLayout->addWidget(header);

        // --- MAIN CONTENT AREA ---
        QHBoxLayout *contentLayout = new QHBoxLayout();
        contentLayout->setContentsMargins(15, 15, 15, 15);
        contentLayout->setSpacing(15);

        // 1. LEFT PANEL (Inputs)
        QGroupBox *grpInput = new QGroupBox("Medicine Details");
        grpInput->setStyleSheet("QGroupBox { font-weight: bold; color: #333333; border: 1px solid #BDC3C7; border-radius: 6px; margin-top: 10px; background-color: white; } "
                                "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }");
        grpInput->setFixedWidth(350);
        QVBoxLayout *leftLayout = new QVBoxLayout(grpInput);

        QGridLayout *formLayout = new QGridLayout();
        formLayout->setVerticalSpacing(15);
        txtId = createInput(formLayout, 0, "ID (Int):");
        txtName = createInput(formLayout, 1, "Name:");
        txtPrice = createInput(formLayout, 2, "Price:");
        txtStock = createInput(formLayout, 3, "Stock:");
        txtExpiry = createInput(formLayout, 4, "Expiry:");
        txtCompany = createInput(formLayout, 5, "Company:");
        leftLayout->addLayout(formLayout);
        leftLayout->addStretch();

        QGridLayout *btnGrid = new QGridLayout();
        btnGrid->setSpacing(10);
        QPushButton *btnAdd = createBtn("ADD", successColor);
        QPushButton *btnUpd = createBtn("UPDATE", warningColor);
        QPushButton *btnDel = createBtn("DELETE", dangerColor);
        QPushButton *btnClr = createBtn("CLEAR", "#6C757D");

        btnGrid->addWidget(btnAdd, 0, 0); btnGrid->addWidget(btnUpd, 0, 1);
        btnGrid->addWidget(btnDel, 1, 0); btnGrid->addWidget(btnClr, 1, 1);
        leftLayout->addLayout(btnGrid);

        connect(btnClr, &QPushButton::clicked, this, &MedicalStore::clearFields);
        connect(btnAdd, &QPushButton::clicked, this, &MedicalStore::addMedicine);
        connect(btnUpd, &QPushButton::clicked, this, &MedicalStore::updateMedicine);
        connect(btnDel, &QPushButton::clicked, this, &MedicalStore::deleteMedicine);

        contentLayout->addWidget(grpInput);

        // 2. CENTER PANEL (Inventory)
        QWidget *centerWidget = new QWidget();
        QVBoxLayout *centerLayout = new QVBoxLayout(centerWidget);
        centerLayout->setContentsMargins(0, 0, 0, 0);

        QGroupBox *grpSearch = new QGroupBox("Inventory Search");
        grpSearch->setStyleSheet(grpInput->styleSheet());
        grpSearch->setFixedHeight(80);
        QVBoxLayout *searchLayout = new QVBoxLayout(grpSearch);
        txtSearch = new QLineEdit();
        txtSearch->setPlaceholderText("Scan ID or type Medicine Name...");
        connect(txtSearch, &QLineEdit::textChanged, this, &MedicalStore::refreshMedicineTable);
        searchLayout->addWidget(txtSearch);
        centerLayout->addWidget(grpSearch);

        tableMedicines = new QTableWidget();
        tableMedicines->setColumnCount(6);
        tableMedicines->setHorizontalHeaderLabels({"ID", "Name", "Price", "Stock", "Expiry", "Company"});
        tableMedicines->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        tableMedicines->setSelectionBehavior(QAbstractItemView::SelectRows);
        tableMedicines->setEditTriggers(QAbstractItemView::NoEditTriggers);
        tableMedicines->setFocusPolicy(Qt::NoFocus);
        connect(tableMedicines, &QTableWidget::cellClicked, this, &MedicalStore::onTableClick);
        centerLayout->addWidget(tableMedicines);

        QPushButton *btnAddToCart = createBtn("ADD SELECTED TO CART  ->", primaryColor);
        btnAddToCart->setFixedHeight(60);
        btnAddToCart->setStyleSheet("QPushButton { background-color: " + primaryColor + "; font-size: 16px; font-weight: bold; color: white; border-radius: 6px; }");
        connect(btnAddToCart, &QPushButton::clicked, this, &MedicalStore::addToCart);
        centerLayout->addWidget(btnAddToCart);

        contentLayout->addWidget(centerWidget);

        // 3. RIGHT PANEL (Cart)
        QGroupBox *grpCart = new QGroupBox("Billing Cart");
        grpCart->setFixedWidth(420);
        grpCart->setStyleSheet(grpInput->styleSheet());
        QVBoxLayout *rightLayout = new QVBoxLayout(grpCart);

        tableCart = new QTableWidget();
        tableCart->setColumnCount(4);
        tableCart->setHorizontalHeaderLabels({"Name", "Price", "Qty", "Total"});
        tableCart->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        rightLayout->addWidget(tableCart);

        QWidget *cartBottom = new QWidget();
        cartBottom->setStyleSheet("background-color: #E8E8E8; border-radius: 6px;");
        QVBoxLayout *cartBottomLayout = new QVBoxLayout(cartBottom);

        lblTotal = new QLabel("Total: Rs 0.00");
        lblTotal->setAlignment(Qt::AlignCenter);
        lblTotal->setStyleSheet("font-size: 26px; font-weight: bold; color: #333333; background: transparent; padding: 10px;");

        QPushButton *btnCheckout = createBtn("CHECKOUT & PRINT", successColor);
        btnCheckout->setFixedHeight(55);
        btnCheckout->setStyleSheet("QPushButton { background-color: " + successColor + "; font-size: 18px; color: white; border-radius: 6px; }");
        connect(btnCheckout, &QPushButton::clicked, this, &MedicalStore::checkout);

        QPushButton *btnRemove = createBtn("Remove Item", dangerColor);
        connect(btnRemove, &QPushButton::clicked, this, &MedicalStore::removeFromCart);

        cartBottomLayout->addWidget(lblTotal);
        cartBottomLayout->addWidget(btnCheckout);
        cartBottomLayout->addWidget(btnRemove);
        rightLayout->addWidget(cartBottom);

        contentLayout->addWidget(grpCart);
        mainLayout->addLayout(contentLayout);

        // --- FOOTER ---
        QLabel *footer = new QLabel("System developed by: Muhammad Faizan U Allah | Contact: mfaizan.w2@gmail.com");
        footer->setAlignment(Qt::AlignCenter);
        footer->setStyleSheet("background-color: #E0E0E0; color: #555555; padding: 6px; font-size: 12px;");
        mainLayout->addWidget(footer);

        refreshMedicineTable("");
    }

private:
    QLineEdit* createInput(QGridLayout *layout, int row, QString labelText) {
        QLabel *lbl = new QLabel(labelText);
        lbl->setStyleSheet("color: #333333; font-weight: bold; background: transparent;");
        QLineEdit *txt = new QLineEdit();
        layout->addWidget(lbl, row, 0);
        layout->addWidget(txt, row, 1);
        return txt;
    }

    QPushButton* createBtn(QString text, QString color) {
        QPushButton *btn = new QPushButton(text);
        btn->setStyleSheet("background-color: " + color + "; color: white; font-weight: bold; border-radius: 4px; padding: 10px;");
        return btn;
    }

    void clearFields() {
        txtId->clear(); txtId->setReadOnly(false);
        txtName->clear(); txtPrice->clear(); txtStock->clear();
        txtExpiry->clear(); txtCompany->clear();
        tableMedicines->clearSelection();
    }

    void addMedicine() {
        if(txtId->text().isEmpty() || txtName->text().isEmpty()) return;
        int id = txtId->text().toInt();
        for(const auto &m : medicineList) {
            if(m.id == id) { QMessageBox::warning(this, "Error", "ID Exists"); return; }
        }
        medicineList.append({id, txtName->text(), txtPrice->text().toDouble(), txtStock->text().toInt(), txtExpiry->text(), txtCompany->text()});
        saveData(); refreshMedicineTable(txtSearch->text()); clearFields();
    }

    void updateMedicine() {
        int row = tableMedicines->currentRow();
        if(row < 0) return;
        int id = txtId->text().toInt();
        for(auto &m : medicineList) {
            if(m.id == id) {
                m.name = txtName->text(); m.price = txtPrice->text().toDouble();
                m.stock = txtStock->text().toInt(); m.expiry = txtExpiry->text(); m.company = txtCompany->text(); break;
            }
        }
        saveData(); refreshMedicineTable(txtSearch->text()); clearFields();
    }

    void deleteMedicine() {
        int row = tableMedicines->currentRow();
        if(row < 0) return;
        int id = tableMedicines->item(row, 0)->text().toInt();
        if(QMessageBox::question(this, "Confirm", "Delete selected?") == QMessageBox::Yes) {
            for(int i=0; i<medicineList.size(); ++i) {
                if(medicineList[i].id == id) { medicineList.removeAt(i); break; }
            }
            saveData(); refreshMedicineTable(txtSearch->text()); clearFields();
        }
    }

    void onTableClick(int row, int /*col*/) {
        txtId->setText(tableMedicines->item(row, 0)->text());
        txtName->setText(tableMedicines->item(row, 1)->text());
        txtPrice->setText(tableMedicines->item(row, 2)->text());
        txtStock->setText(tableMedicines->item(row, 3)->text());
        txtExpiry->setText(tableMedicines->item(row, 4)->text());
        txtCompany->setText(tableMedicines->item(row, 5)->text());
        txtId->setReadOnly(true);
    }

    void refreshMedicineTable(const QString &query) {
        tableMedicines->setRowCount(0);
        for(const auto &m : medicineList) {
            if(query.isEmpty() || m.name.contains(query, Qt::CaseInsensitive) || QString::number(m.id).contains(query)) {
                int row = tableMedicines->rowCount();
                tableMedicines->insertRow(row);
                tableMedicines->setItem(row, 0, new QTableWidgetItem(QString::number(m.id)));
                tableMedicines->setItem(row, 1, new QTableWidgetItem(m.name));
                tableMedicines->setItem(row, 2, new QTableWidgetItem(QString::number(m.price)));
                tableMedicines->setItem(row, 3, new QTableWidgetItem(QString::number(m.stock)));
                tableMedicines->setItem(row, 4, new QTableWidgetItem(m.expiry));
                tableMedicines->setItem(row, 5, new QTableWidgetItem(m.company));

                if(m.stock < 10) {
                    for(int c=0; c<6; c++) tableMedicines->item(row, c)->setBackground(QColor("#FFCDD2"));
                    for(int c=0; c<6; c++) tableMedicines->item(row, c)->setForeground(QBrush(Qt::black));
                }
            }
        }
    }

    void addToCart() {
        int row = tableMedicines->currentRow();
        if(row < 0) { QMessageBox::warning(this, "Warning", "Select Medicine First"); return; }
        int id = tableMedicines->item(row, 0)->text().toInt();
        Medicine *selected = nullptr;
        for(auto &m : medicineList) { if(m.id == id) { selected = &m; break; } }

        if(!selected) return;

        bool ok;
        int qty = QInputDialog::getInt(this, "Add to Cart", "Quantity (Stock: " + QString::number(selected->stock) + "):", 1, 1, 1000, 1, &ok);
        if(ok) {
            int currentInCart = 0;
            for(const auto &c : cartList) if(c.medId == id) currentInCart += c.qty;

            if(qty + currentInCart <= selected->stock) {
                bool found = false;
                for(auto &c : cartList) {
                    if(c.medId == id) { c.qty += qty; found = true; break; }
                }
                if(!found) cartList.append({selected->id, selected->name, selected->price, qty});
                refreshCart();
            } else {
                QMessageBox::warning(this, "Stock", "Not enough stock!");
            }
        }
    }

    void removeFromCart() {
        int row = tableCart->currentRow();
        if(row >= 0) { cartList.removeAt(row); refreshCart(); }
    }

    void refreshCart() {
        tableCart->setRowCount(0);
        double total = 0;
        for(const auto &c : cartList) {
            int row = tableCart->rowCount();
            tableCart->insertRow(row);
            double sub = c.price * c.qty;
            total += sub;
            tableCart->setItem(row, 0, new QTableWidgetItem(c.name));
            tableCart->setItem(row, 1, new QTableWidgetItem(QString::number(c.price)));
            tableCart->setItem(row, 2, new QTableWidgetItem(QString::number(c.qty)));
            tableCart->setItem(row, 3, new QTableWidgetItem(QString::number(sub)));
        }
        lblTotal->setText("Total: Rs " + QString::number(total, 'f', 2));
    }

    void showReceipt(const QString &text) {
        QDialog *dlg = new QDialog(this);
        dlg->setWindowTitle("Print Receipt");
        dlg->resize(400, 500);
        dlg->setStyleSheet("background-color: white;");

        QVBoxLayout *layout = new QVBoxLayout(dlg);

        QTextEdit *edit = new QTextEdit();
        edit->setReadOnly(true);
        edit->setPlainText(text);
        edit->setStyleSheet("border: 1px solid black; font-family: 'Courier New'; font-size: 14px; color: black; background-color: white;");

        QPushButton *btnClose = new QPushButton("Close / Print");
        btnClose->setStyleSheet("background-color: " + primaryColor + "; color: white; padding: 10px; font-weight: bold;");
        connect(btnClose, &QPushButton::clicked, dlg, &QDialog::accept);

        layout->addWidget(edit);
        layout->addWidget(btnClose);
        dlg->exec();
        delete dlg;
    }

    void checkout() {
        if(cartList.isEmpty()) return;
        QString receipt = "      MEDICAL STORE RECEIPT\n";
        receipt += "---------------------------------\n";
        receipt += "Date: " + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm") + "\n";
        receipt += "---------------------------------\n";
        receipt += QString("%1 %2 %3 %4\n").arg("ITEM", -15).arg("PRICE", 6).arg("QTY", 4).arg("TOTAL", 7);
        receipt += "---------------------------------\n";

        double total = 0;
        for(const auto &c : cartList) {
            for(auto &m : medicineList) {
                if(m.id == c.medId) { m.stock -= c.qty; break; }
            }
            double sub = c.price * c.qty;
            total += sub;
            QString name = c.name.left(15);
            receipt += QString("%1 %2 %3 %4\n")
                           .arg(name, -15)
                           .arg(c.price, 6, 'f', 0)
                           .arg(c.qty, 4)
                           .arg(sub, 7, 'f', 1);
        }
        receipt += "---------------------------------\n";
        receipt += "GRAND TOTAL: Rs " + QString::number(total, 'f', 2) + "\n";
        receipt += "---------------------------------\n";
        receipt += "   Thank you for your purchase!\n";

        saveData(); refreshMedicineTable(txtSearch->text());
        cartList.clear(); refreshCart();
        showReceipt(receipt);
    }

    void saveData() {
        QFile file(FILE_NAME);
        if(file.open(QIODevice::WriteOnly)) {
            QDataStream out(&file);
            out << medicineList;
        }
    }

    void loadData() {
        QFile file(FILE_NAME);
        if(file.open(QIODevice::ReadOnly)) {
            QDataStream in(&file);
            in >> medicineList;
        }
    }

    void createBackup(QString type) {
        QDir().mkdir(BACKUP_DIR);
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        QString backupPath = BACKUP_DIR + "/backup_" + type + "_" + timestamp + ".dat";
        if(QFile::copy(FILE_NAME, backupPath)) {
            if(type == "Manual") QMessageBox::information(this, "Backup", "Saved: " + backupPath);
        }
    }

    void performAutoBackup() { createBackup("Auto"); }
};

#include "main.moc"

int main(int argc, char *argv[]) {
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    QApplication a(argc, argv);
    MedicalStore w;
    w.show();
    return a.exec();
}
