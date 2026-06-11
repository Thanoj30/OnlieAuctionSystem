#include <crow.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif

std::ifstream openProjectFile(const std::string& folder, const std::string& filename) {
    std::ifstream file(folder + "/" + filename);
    if (file) {
        return file;
    }

    return std::ifstream(std::string(PROJECT_ROOT) + "/" + folder + "/" + filename);
}

std::string loadFile(const std::string& folder, const std::string& filename) {
    std::ifstream file = openProjectFile(folder, filename);
    if (!file) {
        return "";
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string loadPage(const std::string& filename) {
    std::string page = loadFile("templates", filename);
    if (page.empty()) {
        return "<h1>Page not found</h1><p>Missing template: " + filename + "</p>";
    }

    return page;
}

std::string loadStaticFile(const std::string& filename) {
    return loadFile("static", filename);
}

std::string replaceText(std::string text, const std::string& placeholder, const std::string& value) {
    size_t position = text.find(placeholder);
    while (position != std::string::npos) {
        text.replace(position, placeholder.length(), value);
        position = text.find(placeholder, position + value.length());
    }

    return text;
}

crow::response htmlResponse(const std::string& filename, const std::string& message = "") {
    std::string page = loadPage(filename);
    page = replaceText(page, "{{message}}", message);

    crow::response response(page);
    response.set_header("Content-Type", "text/html; charset=utf-8");
    return response;
}

crow::response templateResponse(
    const std::string& filename,
    const std::vector<std::pair<std::string, std::string>>& replacements
) {
    std::string page = loadPage(filename);

    for (const auto& replacement : replacements) {
        page = replaceText(page, replacement.first, replacement.second);
    }

    crow::response response(page);
    response.set_header("Content-Type", "text/html; charset=utf-8");
    return response;
}

std::string dataFilePath(const std::string& filename) {
    return std::string(PROJECT_ROOT) + "/" + filename;
}

struct User {
    std::string username;
    std::string password;
    std::string fullName;
};

// OOP concept: these structs group related data together.
// User, AuctionItem, and Bid act like simple model classes for this beginner project.
struct AuctionItem {
    int id;
    std::string name;
    std::string description;
    std::string startingPrice;
    std::string sellerUsername;
    std::string highestBid;
    std::string highestBidder;
    std::string status;
    std::string winner;
};

struct Bid {
    int id;
    int itemId;
    std::string bidderUsername;
    std::string amount;
};

std::string htmlEscape(const std::string& text) {
    std::string escaped;

    for (char character : text) {
        if (character == '&') {
            escaped += "&amp;";
        } else if (character == '<') {
            escaped += "&lt;";
        } else if (character == '>') {
            escaped += "&gt;";
        } else if (character == '"') {
            escaped += "&quot;";
        } else {
            escaped += character;
        }
    }

    return escaped;
}

std::vector<std::string> splitText(const std::string& text, char delimiter) {
    std::vector<std::string> parts;
    std::stringstream stream(text);
    std::string part;

    while (std::getline(stream, part, delimiter)) {
        parts.push_back(part);
    }

    return parts;
}

std::vector<User> readUsers() {
    std::vector<User> users;

    // File handling: open users.txt from the project folder and read it line by line.
    // Each registered user is stored as: username,password,full name
    std::ifstream file(dataFilePath("users.txt"));
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::stringstream lineStream(line);
        std::string username;
        std::string password;
        std::string fullName;

        std::getline(lineStream, username, ',');
        std::getline(lineStream, password, ',');
        std::getline(lineStream, fullName);

        if (!username.empty() && !password.empty()) {
            users.push_back({username, password, fullName});
        }
    }

    return users;
}

bool usernameExists(const std::string& username) {
    // Searching: loop through every saved user to find a matching username.
    for (const User& user : readUsers()) {
        if (user.username == username) {
            return true;
        }
    }

    return false;
}

bool saveUser(const std::string& fullName, const std::string& username, const std::string& password) {
    // File handling: append a new user to users.txt without changing existing users.
    std::ofstream file(dataFilePath("users.txt"), std::ios::app);
    if (!file) {
        return false;
    }

    file << username << "," << password << "," << fullName << "\n";
    return true;
}

std::string currentUsername(const crow::request& request) {
    std::string cookies = request.get_header_value("Cookie");
    std::string key = "username=";
    size_t start = cookies.find(key);

    if (start == std::string::npos) {
        return "";
    }

    start += key.length();
    size_t end = cookies.find(';', start);
    if (end == std::string::npos) {
        end = cookies.length();
    }

    return cookies.substr(start, end - start);
}

bool isValidItemInput(const std::string& text) {
    return text.find('|') == std::string::npos &&
           text.find('\n') == std::string::npos &&
           text.find('\r') == std::string::npos;
}

std::vector<AuctionItem> readItems() {
    std::vector<AuctionItem> items;

    // File handling: open items.txt and read one auction item from each line.
    // Item storage format is:
    // item_id|item_name|description|starting_price|seller_username|highest_bid|highest_bidder|status|winner
    std::ifstream file(dataFilePath("items.txt"));
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::vector<std::string> parts = splitText(line, '|');
        if (parts.size() != 5 && parts.size() != 7 && parts.size() != 9) {
            continue;
        }

        try {
            std::string highestBid = parts.size() >= 7 ? parts[5] : parts[3];
            std::string highestBidder = parts.size() >= 7 ? parts[6] : "None";
            std::string status = parts.size() == 9 ? parts[7] : "OPEN";
            std::string winner = parts.size() == 9 ? parts[8] : "Pending";
            items.push_back({std::stoi(parts[0]), parts[1], parts[2], parts[3], parts[4], highestBid, highestBidder, status, winner});
        } catch (...) {
            continue;
        }
    }

    return items;
}

std::vector<AuctionItem> sortedItemsForDisplay() {
    std::vector<AuctionItem> items = readItems();

    // Sorting: show OPEN auctions first, then order items by item ID.
    std::sort(items.begin(), items.end(), [](const AuctionItem& left, const AuctionItem& right) {
        if (left.status != right.status) {
            return left.status == "OPEN";
        }

        return left.id < right.id;
    });

    return items;
}

int nextItemId() {
    int highestId = 0;

    for (const AuctionItem& item : readItems()) {
        if (item.id > highestId) {
            highestId = item.id;
        }
    }

    return highestId + 1;
}

bool saveItem(const AuctionItem& item) {
    // File handling: append the new item to items.txt without changing existing items.
    std::ofstream file(dataFilePath("items.txt"), std::ios::app);
    if (!file) {
        return false;
    }

    // Item storage: each field is separated with | so item descriptions can contain commas.
    file << item.id << "|"
         << item.name << "|"
         << item.description << "|"
         << item.startingPrice << "|"
         << item.sellerUsername << "|"
         << item.highestBid << "|"
         << item.highestBidder << "|"
         << item.status << "|"
         << item.winner << "\n";

    return true;
}

bool rewriteItems(const std::vector<AuctionItem>& items) {
    // File handling: rewrite items.txt after a bid changes one item's highest bid.
    std::ofstream file(dataFilePath("items.txt"));
    if (!file) {
        return false;
    }

    file << "# item_id|item_name|description|starting_price|seller_username|highest_bid|highest_bidder|status|winner\n";
    for (const AuctionItem& item : items) {
        file << item.id << "|"
             << item.name << "|"
             << item.description << "|"
             << item.startingPrice << "|"
             << item.sellerUsername << "|"
             << item.highestBid << "|"
             << item.highestBidder << "|"
             << item.status << "|"
             << item.winner << "\n";
    }

    return true;
}

std::vector<Bid> readBids() {
    std::vector<Bid> bids;

    // File handling: open bids.txt and read every stored bid.
    // Bid storage format is: bid_id|item_id|bidder_username|bid_amount
    std::ifstream file(dataFilePath("bids.txt"));
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::vector<std::string> parts = splitText(line, '|');
        if (parts.size() != 4) {
            continue;
        }

        try {
            bids.push_back({std::stoi(parts[0]), std::stoi(parts[1]), parts[2], parts[3]});
        } catch (...) {
            continue;
        }
    }

    return bids;
}

int nextBidId() {
    int highestId = 0;

    for (const Bid& bid : readBids()) {
        if (bid.id > highestId) {
            highestId = bid.id;
        }
    }

    return highestId + 1;
}

bool saveBid(const Bid& bid) {
    // File handling: append a new bid to bids.txt so the full bid history is kept.
    std::ofstream file(dataFilePath("bids.txt"), std::ios::app);
    if (!file) {
        return false;
    }

    file << bid.id << "|"
         << bid.itemId << "|"
         << bid.bidderUsername << "|"
         << bid.amount << "\n";

    return true;
}

bool amountIsGreaterThan(const std::string& bidAmount, const std::string& currentAmount) {
    try {
        return std::stod(bidAmount) > std::stod(currentAmount);
    } catch (...) {
        return false;
    }
}

std::string renderRecentItemRows(const std::vector<AuctionItem>& items) {
    if (items.empty()) {
        return "<p class=\"small-text\">No auction items have been added yet.</p>";
    }

    std::string html;
    for (const AuctionItem& item : items) {
        html += "<div class=\"item-row\">";
        html += "<span><strong>#" + std::to_string(item.id) + "</strong> " + htmlEscape(item.name) + "</span>";
        html += "<span class=\"status-pill " + htmlEscape(item.status) + "\">" + htmlEscape(item.status) + "</span>";
        html += "<strong>Rs. " + htmlEscape(item.highestBid) + "</strong>";
        html += "</div>";
    }

    return html;
}

std::string renderAuctionCards(const std::vector<AuctionItem>& items, const std::string& viewerUsername) {
    if (items.empty()) {
        return "<p class=\"small-text\">No auction items are available yet.</p>";
    }

    std::string html;
    for (const AuctionItem& item : items) {
        html += "<article class=\"auction-card\">";
        html += "<div class=\"item-image placeholder\"></div>";
        html += "<div class=\"card-topline\"><span class=\"item-id\">Item ID: " + std::to_string(item.id) + "</span>";
        html += "<span class=\"status-pill " + htmlEscape(item.status) + "\">" + htmlEscape(item.status) + "</span></div>";
        html += "<h2>" + htmlEscape(item.name) + "</h2>";
        html += "<p>" + htmlEscape(item.description) + "</p>";
        html += "<div class=\"price-row\"><span>Seller</span><strong>" + htmlEscape(item.sellerUsername) + "</strong></div>";
        html += "<div class=\"price-row\"><span>Starting Price</span><strong>Rs. " + htmlEscape(item.startingPrice) + "</strong></div>";
        html += "<div class=\"price-row\"><span>Current Highest Bid</span><strong>Rs. " + htmlEscape(item.highestBid) + "</strong></div>";
        html += "<div class=\"price-row\"><span>Highest Bidder</span><strong>" + htmlEscape(item.highestBidder) + "</strong></div>";
        html += "<div class=\"price-row\"><span>Winner</span><strong>" + htmlEscape(item.winner) + "</strong></div>";

        if (item.status == "CLOSED") {
            html += "<p class=\"small-text card-note\">This auction is closed.</p>";
        } else {
            html += "<form class=\"bid-form\" action=\"/place-bid\" method=\"POST\">";
            html += "<input type=\"hidden\" name=\"item_id\" value=\"" + std::to_string(item.id) + "\">";
            html += "<label for=\"bid_" + std::to_string(item.id) + "\">Bid Amount</label>";
            html += "<input id=\"bid_" + std::to_string(item.id) + "\" name=\"bid_amount\" type=\"number\" min=\"1\" step=\"1\" placeholder=\"Enter your bid\" required>";
            html += "<button type=\"submit\">Place Bid</button>";
            html += "</form>";
        }

        if (!viewerUsername.empty() && viewerUsername == item.sellerUsername && item.status != "CLOSED") {
            html += "<form class=\"close-form\" action=\"/close-auction\" method=\"POST\">";
            html += "<input type=\"hidden\" name=\"item_id\" value=\"" + std::to_string(item.id) + "\">";
            html += "<button class=\"danger-button\" type=\"submit\">Close Auction</button>";
            html += "</form>";
        }

        html += "</article>";
    }

    return html;
}

bool isValidLogin(const std::string& username, const std::string& password) {
    // Login validation: compare the submitted username and password with users.txt records.
    for (const User& user : readUsers()) {
        if (user.username == username && user.password == password) {
            return true;
        }
    }

    return false;
}

int countOpenItems(const std::vector<AuctionItem>& items) {
    int count = 0;

    for (const AuctionItem& item : items) {
        if (item.status == "OPEN") {
            count++;
        }
    }

    return count;
}

std::string formValue(const crow::request& request, const std::string& fieldName) {
    crow::query_string params = request.get_body_params();
    const char* value = params.get(fieldName);
    return value == nullptr ? "" : std::string(value);
}

crow::response redirectTo(const std::string& path) {
    crow::response response(302);
    response.set_header("Location", path);
    return response;
}

crow::response loginRedirect(const std::string& username) {
    crow::response response = redirectTo("/dashboard");
    response.set_header("Set-Cookie", "username=" + username + "; Path=/; HttpOnly; SameSite=Lax");
    return response;
}

crow::response dashboardResponse(const crow::request& request, const std::string& message = "") {
    std::string username = currentUsername(request);
    if (username.empty() || !usernameExists(username)) {
        return redirectTo("/login");
    }

    std::vector<AuctionItem> items = readItems();
    std::vector<Bid> bids = readBids();
    return templateResponse("dashboard.html", {
        {"{{username}}", htmlEscape(username)},
        {"{{active_count}}", std::to_string(countOpenItems(items))},
        {"{{item_count}}", std::to_string(items.size())},
        {"{{bid_count}}", std::to_string(bids.size())},
        {"{{user_count}}", std::to_string(readUsers().size())},
        {"{{message}}", message},
        {"{{recent_items}}", renderRecentItemRows(sortedItemsForDisplay())}
    });
}

crow::response auctionResponse(const crow::request& request, const std::string& message = "") {
    std::string viewerUsername = currentUsername(request);

    return templateResponse("auction.html", {
        {"{{message}}", message},
        {"{{auction_items}}", renderAuctionCards(sortedItemsForDisplay(), viewerUsername)}
    });
}

crow::response placeBidResponse(const crow::request& request) {
    std::string bidderUsername = currentUsername(request);
    if (bidderUsername.empty() || !usernameExists(bidderUsername)) {
        return redirectTo("/login");
    }

    std::string itemIdText = formValue(request, "item_id");
    std::string bidAmount = formValue(request, "bid_amount");

    if (itemIdText.empty() || bidAmount.empty()) {
        return auctionResponse(request, "<p class=\"error-message\">Please enter a bid amount.</p>");
    }

    if (!isValidItemInput(bidAmount)) {
        return auctionResponse(request, "<p class=\"error-message\">Bid amount contains invalid characters.</p>");
    }

    int itemId = 0;
    try {
        itemId = std::stoi(itemIdText);
    } catch (...) {
        return auctionResponse(request, "<p class=\"error-message\">Invalid item selected.</p>");
    }

    std::vector<AuctionItem> items = readItems();
    for (AuctionItem& item : items) {
        if (item.id != itemId) {
            continue;
        }

        // Bid validation: closed auctions do not accept new bids.
        if (item.status == "CLOSED") {
            return auctionResponse(request, "<p class=\"error-message\">This auction is closed. No more bids are allowed.</p>");
        }

        // Bid validation: sellers cannot bid on their own auction items.
        if (item.sellerUsername == bidderUsername) {
            return auctionResponse(request, "<p class=\"error-message\">You cannot bid on your own item.</p>");
        }

        // Bid validation: every new bid must be greater than the current highest bid.
        if (!amountIsGreaterThan(bidAmount, item.highestBid)) {
            return auctionResponse(request, "<p class=\"error-message\">Bid amount must be greater than the current highest bid.</p>");
        }

        Bid bid = {nextBidId(), item.id, bidderUsername, bidAmount};
        if (!saveBid(bid)) {
            return auctionResponse(request, "<p class=\"error-message\">Could not save bid. Please try again.</p>");
        }

        // Highest bid update: after storing the bid history, update the winning bid fields.
        item.highestBid = bidAmount;
        item.highestBidder = bidderUsername;

        if (!rewriteItems(items)) {
            return auctionResponse(request, "<p class=\"error-message\">Bid saved, but item highest bid could not be updated.</p>");
        }

        return auctionResponse(request, "<p class=\"success-message\">Bid placed successfully.</p>");
    }

    return auctionResponse(request, "<p class=\"error-message\">Auction item was not found.</p>");
}

crow::response closeAuctionResponse(const crow::request& request) {
    std::string sellerUsername = currentUsername(request);
    if (sellerUsername.empty() || !usernameExists(sellerUsername)) {
        return redirectTo("/login");
    }

    std::string itemIdText = formValue(request, "item_id");
    int itemId = 0;

    try {
        itemId = std::stoi(itemIdText);
    } catch (...) {
        return auctionResponse(request, "<p class=\"error-message\">Invalid item selected.</p>");
    }

    std::vector<AuctionItem> items = readItems();
    for (AuctionItem& item : items) {
        if (item.id != itemId) {
            continue;
        }

        if (item.sellerUsername != sellerUsername) {
            return auctionResponse(request, "<p class=\"error-message\">Unauthorized close attempt. You can close only your own auction item.</p>");
        }

        if (item.status == "CLOSED") {
            return auctionResponse(request, "<p class=\"error-message\">Auction is already closed.</p>");
        }

        // Winner selection: the current highest bidder becomes the winner.
        // If nobody has bid yet, there is no winner for this auction.
        bool hasWinner = item.highestBidder != "None" && !item.highestBidder.empty();
        item.winner = hasWinner ? item.highestBidder : "No winner";

        // Item status update: mark the item CLOSED and rewrite items.txt.
        item.status = "CLOSED";

        if (!rewriteItems(items)) {
            return auctionResponse(request, "<p class=\"error-message\">Could not close auction. Please try again.</p>");
        }

        if (hasWinner) {
            return auctionResponse(request, "<p class=\"success-message\">Auction closed. Winner: " + htmlEscape(item.winner) + ".</p>");
        }

        return auctionResponse(request, "<p class=\"success-message\">Auction closed. No winner.</p>");
    }

    return auctionResponse(request, "<p class=\"error-message\">Auction item was not found.</p>");
}

int main() {
    crow::SimpleApp app;

    // Routing: each CROW_ROUTE connects a URL to a C++ function/lambda.
    // GET routes display pages, while POST routes process form submissions.
    CROW_ROUTE(app, "/")([] {
        return htmlResponse("index.html");
    });

    CROW_ROUTE(app, "/login")([](const crow::request& request) {
        const char* registered = request.url_params.get("registered");
        crow::response response;
        if (registered != nullptr) {
            response = htmlResponse("login.html", "<p class=\"success-message\">Registration successful. Please login.</p>");
        } else {
            response = htmlResponse("login.html");
        }

        response.set_header("Set-Cookie", "username=; Path=/; Max-Age=0; HttpOnly; SameSite=Lax");
        return response;
    });

    CROW_ROUTE(app, "/logout")([] {
        crow::response response = redirectTo("/login");
        response.set_header("Set-Cookie", "username=; Path=/; Max-Age=0; HttpOnly; SameSite=Lax");
        return response;
    });

    CROW_ROUTE(app, "/register")([] {
        return htmlResponse("register.html");
    });

    // Route handling: POST /register receives form data and stores a new user.
    CROW_ROUTE(app, "/register").methods(crow::HTTPMethod::Post)([](const crow::request& request) {
        std::string fullName = formValue(request, "name");
        std::string username = formValue(request, "username");
        std::string password = formValue(request, "password");

        if (fullName.empty() || username.empty() || password.empty()) {
            return htmlResponse("register.html", "<p class=\"error-message\">Please fill in all fields.</p>");
        }

        if (usernameExists(username)) {
            return htmlResponse("register.html", "<p class=\"error-message\">Username already exists. Please choose another username.</p>");
        }

        if (!saveUser(fullName, username, password)) {
            return htmlResponse("register.html", "<p class=\"error-message\">Could not save user. Please try again.</p>");
        }

        return redirectTo("/login?registered=1");
    });

    // Route handling: POST /login receives form data and checks it against users.txt.
    CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::Post)([](const crow::request& request) {
        std::string username = formValue(request, "username");
        std::string password = formValue(request, "password");

        if (isValidLogin(username, password)) {
            return loginRedirect(username);
        }

        return htmlResponse("login.html", "<p class=\"error-message\">Invalid username or password.</p>");
    });

    CROW_ROUTE(app, "/dashboard")([](const crow::request& request) {
        return dashboardResponse(request);
    });

    // Route handling: POST /add-item receives item details from the dashboard form.
    CROW_ROUTE(app, "/add-item").methods(crow::HTTPMethod::Post)([](const crow::request& request) {
        std::string sellerUsername = currentUsername(request);
        if (sellerUsername.empty()) {
            return redirectTo("/login");
        }

        std::string itemName = formValue(request, "item_name");
        std::string description = formValue(request, "description");
        std::string startingPrice = formValue(request, "starting_price");

        if (itemName.empty() || description.empty() || startingPrice.empty()) {
            return dashboardResponse(request, "<p class=\"error-message\">Please fill in all item fields.</p>");
        }

        if (!isValidItemInput(itemName) || !isValidItemInput(description) || !isValidItemInput(startingPrice)) {
            return dashboardResponse(request, "<p class=\"error-message\">Item details cannot contain the | symbol.</p>");
        }

        AuctionItem item = {
            nextItemId(),
            itemName,
            description,
            startingPrice,
            sellerUsername,
            startingPrice,
            "None",
            "OPEN",
            "Pending"
        };

        if (!saveItem(item)) {
            return dashboardResponse(request, "<p class=\"error-message\">Could not save item. Please try again.</p>");
        }

        return dashboardResponse(request, "<p class=\"success-message\">Auction item added successfully.</p>");
    });

    CROW_ROUTE(app, "/auction")([](const crow::request& request) {
        return auctionResponse(request);
    });

    // Route handling: POST /place-bid receives a bid from an auction item card.
    CROW_ROUTE(app, "/place-bid").methods(crow::HTTPMethod::Post)([](const crow::request& request) {
        return placeBidResponse(request);
    });

    // Route handling: POST /close-auction lets a seller close only their own item.
    CROW_ROUTE(app, "/close-auction").methods(crow::HTTPMethod::Post)([](const crow::request& request) {
        return closeAuctionResponse(request);
    });

    CROW_ROUTE(app, "/static/style.css")([] {
        std::string css = loadStaticFile("style.css");
        if (css.empty()) {
            return crow::response(404, "CSS file not found");
        }

        crow::response response(css);
        response.set_header("Content-Type", "text/css");
        return response;
    });

    app.bindaddr("127.0.0.1").port(18080).run();
    //app.port(18080).multithreaded().run();
}
