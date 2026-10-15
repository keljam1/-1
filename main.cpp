#include "Book.h"
#include "BorrowRecord.h"
#include "LibraryManager.h"
#include <sstream>

// ========== Book 实现 ==========
Book::Book(const string& t, const string& i, const string& a)
    : title(t), isbn(i), author(a) {}

string Book::getTitle() const { return title; }
string Book::getIsbn() const { return isbn; }
string Book::getAuthor() const { return author; }
bool Book::isAvailableNow() const { return isAvailable; }
void Book::setAvailable(bool f) { isAvailable = f; }

string Book::toCSV() const {
    return "B," + isbn + "," + title + "," + author + ",," + (isAvailable ? "1" : "0");
}
void Book::display() const {
    cout << "书名:" << title << " 作者:" << author << " ISBN:" << isbn
         << (isAvailable ? " [在馆]" : " [已借出]") << endl;
}

// ========== PaperBook 实现 ==========
PaperBook::PaperBook(const string& t, const string& i, const string& a, int p)
    : Book(t,i,a), pages(p) {}

string PaperBook::toCSV() const override {
    return "P," + isbn + "," + title + "," + author + ","
         + to_string(pages) + "," + (isAvailable ? "1" : "0");
}
void PaperBook::display() const override {
    cout << "[纸质书 " << pages << "页] ";
    Book::display();
}

// ========== EBook 实现 ==========
EBook::EBook(const string& t, const string& i, const string& a, double f)
    : Book(t,i,a), fileSizeMB(f) {}

string EBook::toCSV() const override {
    ostringstream oss;
    oss << "E," << isbn << "," << title << "," << author << ","
        << fileSizeMB << "," << (isAvailable ? "1" : "0");
    return oss.str();
}
void EBook::display() const override {
    cout << "[电子书 " << fileSizeMB << "MB] ";
    Book::display();
}

// ========== parseBook 函数实现 ==========
unique_ptr<Book> parseBook(const string& line) {
    vector<string> f;
    size_t start = 0;
    while (true) {
        size_t pos = line.find(',', start);
        if (pos == string::npos) { f.push_back(line.substr(start)); break; }
        f.push_back(line.substr(start, pos - start));
        start = pos + 1;
    }
    if (f.size() < 6) return nullptr;

    const string& type = f[0];
    const string& isbn = f[1];
    const string& title = f[2];
    const string& author = f[3];
    const string& param = f[4];
    bool avail = (f[5] == "1");

    unique_ptr<Book> b;
    try {
        if (type == "P") {
            b = make_unique<PaperBook>(title, isbn, author, stoi(param));
        } else if (type == "E") {
            b = make_unique<EBook>(title, isbn, author, stod(param));
        } else {
            return nullptr;
        }
    } catch (...) {
        return nullptr;
    }
    b->setAvailable(avail);
    return b;
}

// ========== BorrowRecord 实现 ==========
BorrowRecord::BorrowRecord(const string& i, const string& b, const string& d)
    : isbn(i), borrower(b), date(d) {}

string BorrowRecord::getIsbn() const { return isbn; }
string BorrowRecord::getBorrower() const { return borrower; }
string BorrowRecord::getDate() const { return date; }
string BorrowRecord::toCSV() const { return isbn + "," + borrower + "," + date; }

// ========== LibraryManager 实现 ==========
LibraryManager::LibraryManager(string bf, string rf)
    : booksFile(move(bf)), recordsFile(move(rf)) {}

bool LibraryManager::addBook(unique_ptr<Book> b) {
    if (findBook(b->getIsbn()) != nullptr) {
        cout << "添加失败：ISBN " << b->getIsbn() << " 已存在！" << endl;
        return false;
    }
    books.push_back(move(b));
    saveData();
    return true;
}
Book* LibraryManager::findBook(const string& isbn) {
    auto it = find_if(books.begin(), books.end(),
        [&](const unique_ptr<Book>& b) { return b->getIsbn() == isbn; });
    return (it == books.end()) ? nullptr : it->get();
}
bool LibraryManager::borrowBook(const string& isbn, const string& borrower, const string& date) {
    Book* b = findBook(isbn);
    if (b == nullptr) {
        cout << "借阅失败：未找到该图书！" << endl;
        return false;
    }
    if (!b->isAvailableNow()) {
        cout << "借阅失败：图书已被借出！" << endl;
        return false;
    }
    b->setAvailable(false);
    records.emplace_back(isbn, borrower, date);
    saveData();
    cout << "借阅成功！" << endl;
    return true;
}
bool LibraryManager::returnBook(const string& isbn) {
    Book* b = findBook(isbn);
    if (b == nullptr || b->isAvailableNow()) {
        cout << "归还失败：该书不存在或未被借出！" << endl;
        return false;
    }
    b->setAvailable(true);
    auto it = find_if(records.begin(), records.end(),
        [&](const BorrowRecord& r) { return r.getIsbn() == isbn; });
    if (it != records.end()) records.erase(it);
    saveData();
    cout << "归还成功！" << endl;
    return true;
}
void LibraryManager::listBooks() const {
    if (books.empty()) { cout << "（暂无藏书）" << endl; return; }
    for (const auto& b : books) b->display();
}
void LibraryManager::listRecords() const {
    if (records.empty()) { cout << "（当前无未归还记录）" << endl; return; }
    for (const auto& r : records) {
        cout << "ISBN:" << r.getIsbn() << " 借阅人:" << r.getBorrower()
             << " 日期:" << r.getDate() << endl;
    }
}
void LibraryManager::saveData() const {
    ofstream fout(booksFile);
    if (!fout) { cerr << "警告：无法写入 " << booksFile << endl; return; }
    for (const auto& b : books) fout << b->toCSV() << '\n';
    fout.close();

    ofstream rout(recordsFile);
    if (!rout) { cerr << "警告：无法写入 " << recordsFile << endl; return; }
    for (const auto& r : records) rout << r.toCSV() << '\n';
    rout.close();
}
void LibraryManager::loadData() {
    books.clear();
    records.clear();
    ifstream fin(booksFile);
    string line;
    while (getline(fin, line)) {
        if (line.empty()) continue;
        auto b = parseBook(line);
        if (b) books.push_back(move(b));
        else cerr << "警告：跳过无法解析的行: " << line << endl;
    }
    ifstream rin(recordsFile);
    while (getline(rin, line)) {
        if (line.empty()) continue;
        size_t c1 = line.find(',');
        size_t c2 = line.find(',', c1 + 1);
        if (c1 == string::npos || c2 == string::npos) continue;
        string isbn = line.substr(0, c1);
        string borrower = line.substr(c1 + 1, c2 - c1 - 1);
        string date = line.substr(c2 + 1);
        Book* b = findBook(isbn);
        if (b == nullptr) {
            cerr << "警告：记录对应书籍不存在，已忽略: " << isbn << endl;
            continue;
        }
        b->setAvailable(false);
        records.emplace_back(isbn, borrower, date);
    }
}

// ========== main入口 ==========
int main() {
    LibraryManager lib;
    lib.loadData();

    if (lib.findBook("978-7-111-00001-1") == nullptr) {
        lib.addBook(make_unique<PaperBook>("C++ Primer", "978-7-111-00001-1", "Lippman", 976));
        lib.addBook(make_unique<PaperBook>("Effective C++", "978-7-111-00002-8", "Meyers", 320));
        lib.addBook(make_unique<EBook>("深度探索C++对象模型", "978-7-111-00003-5", "侯捷", 2.4));
    }

    cout << "===== 当前馆藏 =====" << endl;
    lib.listBooks();

    cout << "\n===== 借书 =====" << endl;
    lib.borrowBook("978-7-111-00001-1", "张三", "2026-10-12");
    lib.borrowBook("978-7-111-00001-1", "李四", "2026-10-12");

    cout << "\n===== 借出后 =====" << endl;
    lib.listBooks();
    lib.listRecords();

    cout << "\n===== 还书 =====" << endl;
    lib.returnBook("978-7-111-00001-1");
    lib.returnBook("978-7-111-00001-1");

    cout << "\n===== 归还后 =====" << endl;
    lib.listBooks();
    return 0;
}