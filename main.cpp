// include
// --------------------------------------------------------------------
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <iostream>
#include <fstream>
#include <random>
#include <cmath>
#include <vector>
// --------------------------------------------------------------------

/*
Run command:

g++ main.cpp -I/opt/homebrew/Cellar/sfml/2.6.1/include -o prog -L/opt/homebrew/Cellar/sfml/2.6.1/lib -lsfml-graphics -lsfml-window -lsfml-system

*/

enum GameState {
    MAIN_MENU,
    OPTIONS,
    SAVES,
    PLAYING,
    PAUSED_MENU,
    GAME_OVER,
};

GameState current_state = MAIN_MENU;

// setup
// --------------------------------------------------------------------
sf::RenderWindow window(sf::VideoMode(800.f, 800.f), "Polygon Perihelion", sf::Style::Close);
sf::ConvexShape player(3UL);
sf::ConvexShape player_hitbox(3UL);
std::vector<sf::CircleShape> trail;
sf::Vector2f center(window.getSize().x / 2, window.getSize().y / 2);
sf::Font font;
int orientation = 0;
// --------------------------------------------------------------------

// utility functions
// --------------------------------------------------------------------
int randint(int min, int max) {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(min, max);
    return dist(rng);
}

void rotate(sf::ConvexShape &cs, int p_count, double degrees, int &o = orientation) {
    std::vector<float> x_coords;
    std::vector<float> y_coords;
    double h = 0;
    double k = 0;

    for(int i = 0; i < p_count; i++) {
        x_coords.push_back(cs.getPoint(i).x);
        h += cs.getPoint(i).x;
        y_coords.push_back(cs.getPoint(i).y);
        k += cs.getPoint(i).y;
    }

    h /= p_count;
    k /= p_count;

    double radians = degrees * (M_PI / 180);

    for(int i = 0; i < p_count; i++) {
        float x = x_coords[i];
        float y = y_coords[i];

        cs.setPoint(i, sf::Vector2f(h + cos(radians) * (x - h) - sin(radians) * (y - k), k + sin(radians) * (x - h) + cos(radians) * (y - k)));
    }

    if(&cs == &player) {
        o += degrees;
    }
}

int normalize_angle(int angle) {
    angle %= 360;

    if(angle < 0) {
        angle += 360;
    }

    return angle;
}

bool projections_overlap(const sf::ConvexShape &a, const sf::ConvexShape &b) {
    for(int i = 0; i < a.getPointCount(); i++) {
        sf::Vector2f point1 = a.getTransform().transformPoint(a.getPoint(i));
        sf::Vector2f point2 = a.getTransform().transformPoint(a.getPoint((i + 1) % a.getPointCount()));
        sf::Vector2f edge = point2 - point1;
        sf::Vector2f axis(-edge.y, edge.x);

        float a_min_projection = INFINITY;
        float a_max_projection = -INFINITY;

        for(int j = 0; j < a.getPointCount(); j++) {
            sf::Vector2f point = a.getTransform().transformPoint(a.getPoint(j));
            float projection = point.x * axis.x + point.y * axis.y;
            a_min_projection = std::min(a_min_projection, projection);
            a_max_projection = std::max(a_max_projection, projection);
        }

        float b_min_projection = INFINITY;
        float b_max_projection = -INFINITY;

        for(int j = 0; j < b.getPointCount(); j++) {
            sf::Vector2f point = b.getTransform().transformPoint(b.getPoint(j));
            float projection = point.x * axis.x + point.y * axis.y;
            b_min_projection = std::min(b_min_projection, projection);
            b_max_projection = std::max(b_max_projection, projection);
        }

        if(a_max_projection < b_min_projection || b_max_projection < a_min_projection) {
            return false;
        }
    }

    return true;
}

bool projections_overlap(const sf::CircleShape &a, const sf::ConvexShape &b) {
    sf::Vector2f circle_center = a.getPosition() + sf::Vector2f(a.getRadius(), a.getRadius());

    for(int i = 0; i < b.getPointCount(); i++) {
        sf::Vector2f point1 = b.getTransform().transformPoint(b.getPoint(i));
        sf::Vector2f point2 = b.getTransform().transformPoint(b.getPoint((i + 1) % b.getPointCount()));
        sf::Vector2f edge = point2 - point1;
        sf::Vector2f axis(-edge.y, edge.x);

        float b_min_projection = INFINITY;
        float b_max_projection = -INFINITY;

        for(int j = 0; j < b.getPointCount(); j++) {
            sf::Vector2f point = b.getTransform().transformPoint(b.getPoint(j));
            float projection = point.x * axis.x + point.y * axis.y;
            b_min_projection = std::min(b_min_projection, projection);
            b_max_projection = std::max(b_max_projection, projection);
        }

        float center_projection = circle_center.x * axis.x + circle_center.y * axis.y;
        float radius_projection = a.getRadius() * sqrt(axis.x * axis.x + axis.y * axis.y);
        float a_min_projection = center_projection - radius_projection;
        float a_max_projection = center_projection + radius_projection;

        if(a_max_projection < b_min_projection || b_max_projection < a_min_projection) {
            return false;
        }
    }

    float closest_distance = INFINITY;
    sf::Vector2f closest_vertex;

    for(int i = 0; i < b.getPointCount(); i++) {
        sf::Vector2f vertex = b.getTransform().transformPoint(b.getPoint(i));
        float distance = sqrt(pow(vertex.x - circle_center.x, 2) + pow(vertex.y - circle_center.y, 2));

        if(distance < closest_distance) {
            closest_distance = distance;
            closest_vertex = vertex;
        }
    }

    sf::Vector2f axis = closest_vertex - circle_center;
    float axis_length = sqrt(axis.x * axis.x + axis.y * axis.y);

    if(axis_length > 0.f) {
        axis /= axis_length;

        float circle_projection = circle_center.x * axis.x + circle_center.y * axis.y;
        float a_min_projection = circle_projection - a.getRadius();
        float a_max_projection = circle_projection + a.getRadius();

        float b_min_projection = INFINITY;
        float b_max_projection = -INFINITY;

        for(int i = 0; i < b.getPointCount(); i++) {
            sf::Vector2f point = b.getTransform().transformPoint(b.getPoint(i));
            float projection = point.x * axis.x + point.y * axis.y;
            b_min_projection = std::min(b_min_projection, projection);
            b_max_projection = std::max(b_max_projection, projection);
        }

        if(a_max_projection < b_min_projection || b_max_projection < a_min_projection) {
            return false;
        }
    }

    return true;
}

bool check_collision(const sf::ConvexShape &a, const sf::ConvexShape &b) {
    return projections_overlap(a, b) && projections_overlap(b, a);
}

bool check_collision(const sf::CircleShape &a, const sf::ConvexShape &b) {
    return projections_overlap(a, b);
}

bool check_collision(const sf::ConvexShape &a, const sf::CircleShape &b) {
    return projections_overlap(b, a);
}

// --------------------------------------------------------------------

// score
// --------------------------------------------------------------------
int score = 0;
int high_score = 0;
// --------------------------------------------------------------------
// classes
// --------------------------------------------------------------------
class Projectile : public sf::ConvexShape {

    public:

        sf::ConvexShape creator;
        int direction;
        int p_type;
        int power;
        float travel_speed;
        float energy_cost;
        sf::ConvexShape hitbox;

        Projectile(sf::ConvexShape creator, int p_type) {
            this -> creator = creator;
            this -> direction = orientation;
            this -> p_type = p_type;

            this -> setPointCount(4UL);
            this -> hitbox.setPointCount(4UL);

            float x_size;
            float y_size;
            sf::Color color;

            switch(p_type) {
                case 1:
                    x_size = 10.f;
                    y_size = 4.f;
                    color = sf::Color::Green;
                    this -> power = 200;
                    this -> travel_speed = 1.f;
                    this -> energy_cost = 10.f;
                    break;

                case 2:
                    x_size = 14.f;
                    y_size = 7.f;
                    color = sf::Color(255, 0, 255, 255);
                    this -> power = 500;
                    this -> travel_speed = 0.7f;
                    this -> energy_cost = 25.f;
                    break;

                default:
                    x_size = 10.f;
                    y_size = 4.f;
                    color = sf::Color::Green;
                    this -> power = 200;
                    this -> travel_speed = 1.f;
                    this -> energy_cost = 10.f;
                    break;
            }

            this -> setPoint(0, sf::Vector2f(0.f, 0.f));
            this -> setPoint(1, sf::Vector2f(x_size, y_size));
            this -> setPoint(2, sf::Vector2f(x_size * 2, 0.f));
            this -> setPoint(3, sf::Vector2f(x_size, -y_size));
            this -> setFillColor(color);

            this -> hitbox.setPoint(0, sf::Vector2f(0.f, -y_size));
            this -> hitbox.setPoint(1, sf::Vector2f(x_size * 2, -y_size));
            this -> hitbox.setPoint(2, sf::Vector2f(x_size * 2, y_size));
            this -> hitbox.setPoint(3, sf::Vector2f(0.f, y_size));
            this -> hitbox.setFillColor(sf::Color::Transparent);
            this -> hitbox.setOutlineThickness(2.f);
            this -> hitbox.setOutlineColor(sf::Color::Red);

            this -> setOrigin(x_size, 0.f);
            this -> hitbox.setOrigin(x_size, 0.f);
            this -> setRotation(direction);
            this -> hitbox.setRotation(direction);

            sf::Vector2f tip = player.getPoint(1) + player.getPosition();
            this -> setPosition(tip);
            this -> hitbox.setPosition(tip);
        }

        void travel(float speed) {
            float radians = direction * (M_PI / 180);
            this -> move((speed * 2) * this -> travel_speed * cos(radians), (speed * 2) * this -> travel_speed * sin(radians));
            this -> hitbox.move((speed * 2) * this -> travel_speed * cos(radians), (speed * 2) * this -> travel_speed * sin(radians));
        }
};


class Asteroid : public sf::ConvexShape {

    public:

        int durability;
        float spawny;
        float radius;
        float speed;
        float density_multiplier;
        sf::CircleShape hitbox;

        Asteroid(float x, float y) {
            setPointCount(randint(9, 12));

            int spawn = randint(0, 200);
            this -> spawny = y;

            if(spawn < 80) {
                this -> radius = randint(30, 40);
            }
            else if(spawn < 160) {
                this -> radius = randint(60, 75);
            }
            else {
                this -> radius = randint(90, 100);
            }

            // --------------------------------------------------------
            // Asteroid density
            // --------------------------------------------------------
            int density_roll = randint(0, 100);

            if(score < 10) {
                if(density_roll < 75) {
                    this -> density_multiplier = 1.f;
                }
                else if(density_roll < 95) {
                    this -> density_multiplier = 1.5f;
                }
                else {
                    this -> density_multiplier = 2.f;
                }
            }
            else if(score < 25) {
                if(density_roll < 50) {
                    this -> density_multiplier = 1.f;
                }
                else if(density_roll < 85) {
                    this -> density_multiplier = 1.5f;
                }
                else if(density_roll < 98) {
                    this -> density_multiplier = 2.f;
                }
                else {
                    this -> density_multiplier = 2.5f;
                }
            }
            else {
                if(density_roll < 25) {
                    this -> density_multiplier = 1.f;
                }
                else if(density_roll < 65) {
                    this -> density_multiplier = 1.5f;
                }
                else if(density_roll < 90) {
                    this -> density_multiplier = 2.f;
                }
                else {
                    this -> density_multiplier = 2.5f;
                }
            }

            float hitbox_radius = radius * 0.88;
            this -> hitbox.setRadius(hitbox_radius);
            this -> hitbox.setPosition(this -> getPosition().x + 900.f - hitbox_radius, this -> getPosition().y + this -> spawny - hitbox_radius);
            this -> hitbox.setFillColor(sf::Color::Transparent);
            this -> hitbox.setOutlineThickness(2.f);
            this -> hitbox.setOutlineColor(sf::Color::Red);

            durability = static_cast<int>(this -> radius * 10 * this -> density_multiplier);
            speed = 10.f / radius;

            for(int i = 0; i < getPointCount(); ++i) {
                float angle = (i * 2 * M_PI) / getPointCount();
                float offsetX = cos(angle) * radius;
                float offsetY = sin(angle) * radius;
                setPoint(i, sf::Vector2f(x + offsetX, y + offsetY));
            }

            int grey_shade;

            if(this -> density_multiplier == 1.f) {
                grey_shade = randint(150, 211);
            }
            else if(this -> density_multiplier == 1.5f) {
                grey_shade = randint(110, 149);
            }
            else if(this -> density_multiplier == 2.f) {
                grey_shade = randint(80, 109);
            }
            else {
                grey_shade = randint(50, 79);
            }

            setFillColor(sf::Color(grey_shade, grey_shade, grey_shade, 255));
        }

        void move(float x, float y) {
            sf::ConvexShape::move(x, y);
            hitbox.move(x, y);
        }
};
// --------------------------------------------------------------------

// cooldowns, intervals, the whole enchilada
// --------------------------------------------------------------------
int cooldown = 0;
int interval = 50;
int shot_time = interval;
int spawn_rate = 500;
int spawn_chance = spawn_rate;
int swap_speed = 40;
int swap_count = swap_speed;
float speed = 1.f;
bool show_hitboxes = false;
int interval_count = 0;

const int SCORE_INTERVAL = 60;
// --------------------------------------------------------------------

// energy
// --------------------------------------------------------------------
float energy = 100.f;

const float MAX_ENERGY = 100.f;
const float ENERGY_REGEN = 0.08f;
// --------------------------------------------------------------------

// explosion
// --------------------------------------------------------------------
sf::CircleShape explosion;
sf::Clock explosion_clock;
bool explosion_active = false;
sf::Color explosion_color = sf::Color::White;
float explosion_start_radius = 5.f;

const float EXPLOSION_DURATION = 0.5f;
const float EXPLOSION_MAX_RADIUS = 75.f;
// --------------------------------------------------------------------

// object vectors
// --------------------------------------------------------------------
std::vector<Projectile> projs;
std::vector<Asteroid> belt;
// --------------------------------------------------------------------

// menu functions
// --------------------------------------------------------------------
void menu_mode() {
    window.clear();

    sf::Text title("Polygon Perihelion", font, 50);
    title.setFillColor(sf::Color::White);
    title.setPosition(window.getSize().x / 2 - title.getLocalBounds().width / 2, 100);
    window.draw(title);

    sf::Text instruction("Press Enter to Start", font, 30);
    instruction.setFillColor(sf::Color::Green);
    instruction.setPosition(window.getSize().x / 2 - instruction.getLocalBounds().width / 2, window.getSize().y / 2);
    window.draw(instruction);
    window.display();
}

void display_explosion(sf::Vector2f position, sf::Color color, float radius) {
    explosion.setRadius(radius);
    explosion.setOrigin(radius, radius);
    explosion.setPosition(position);
    explosion.setFillColor(sf::Color::Transparent);
    explosion.setOutlineThickness(6.f);
    explosion.setOutlineColor(color);

    explosion_color = color;
    explosion_start_radius = radius;
    explosion_active = true;
    explosion_clock.restart();
}

void reset_player() {
    player.setPoint(0, center);
    player.setPoint(1, sf::Vector2f(player.getPoint(0).x + 30.f, player.getPoint(0).y + 7.5f));
    player.setPoint(2, sf::Vector2f(player.getPoint(0).x, player.getPoint(0).y + 15.f));

    player_hitbox.setPoint(0, player.getPoint(0));
    player_hitbox.setPoint(1, player.getPoint(1));
    player_hitbox.setPoint(2, player.getPoint(2));

    player.setPosition(0.f, 0.f);
    player_hitbox.setPosition(0.f, 0.f);
    orientation = 0;
}

void game_over() {
    if(score > high_score) {
        high_score = score;
    }

    // ------------------------------------------------------------
    // Explosion
    // ------------------------------------------------------------
    if(explosion_active) {
        float elapsed = explosion_clock.getElapsedTime().asSeconds();
        float progress = elapsed / EXPLOSION_DURATION;

        if(progress >= 1.f) {
            explosion_active = false;
        }
        else {
            float radius = explosion_start_radius + (EXPLOSION_MAX_RADIUS - explosion_start_radius) * progress;
            explosion.setRadius(radius);
            explosion.setOrigin(radius, radius);

            sf::Uint8 alpha = static_cast<sf::Uint8>(255.f * (1.f - progress));

            explosion.setOutlineColor(sf::Color(explosion_color.r, explosion_color.g, explosion_color.b, alpha));
            window.draw(explosion);
        }
    }

    sf::Text game_over_text("Game Over", font, 50);
    game_over_text.setFillColor(sf::Color::White);
    game_over_text.setPosition(window.getSize().x / 2 - game_over_text.getLocalBounds().width / 2, window.getSize().y / 4);
    window.draw(game_over_text);

    sf::Text restart_text("Press Enter to Restart", font, 30);
    restart_text.setFillColor(sf::Color::White);
    restart_text.setPosition(window.getSize().x / 2 - restart_text.getLocalBounds().width / 2, window.getSize().y / 2);
    window.draw(restart_text);

    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Enter)) {
        current_state = MAIN_MENU;
        reset_player();
        projs.clear();
        belt.clear();
        trail.clear();
        explosion_active = false;
        score = 0;
        energy = MAX_ENERGY;

        cooldown = 0;
        interval = 50;
        shot_time = interval;
        spawn_rate = 500;
        spawn_chance = spawn_rate;
        swap_speed = 40;
        swap_count = swap_speed;
        speed = 1.f;
    }

    window.display();
}

void player_death() {
    current_state = GAME_OVER;

    display_explosion(sf::Vector2f(player.getPosition().x + center.x, player.getPosition().y + center.y), sf::Color::Red, 2.f);
    display_explosion(sf::Vector2f(player.getPosition().x + center.x, player.getPosition().y + center.y), sf::Color::Cyan, 5.f);
}

// --------------------------------------------------------------------

int main() {

    // setup player & window
    // --------------------------------------------------------------------
    window.setFramerateLimit(240);

    if(!font.loadFromFile("Orbitron-VariableFont_wght.ttf")) {
        std::cerr << "Error loading font" << std::endl;
        return -1;
    }

    player.setFillColor(sf::Color(230, 0, 0, 255));
    player.setOutlineThickness(2.f);
    player.setOutlineColor(sf::Color(211, 211, 211, 255));

    player.setPoint(0, center);
    player.setPoint(1, sf::Vector2f(player.getPoint(0).x + 30.f, player.getPoint(0).y + 7.5f));
    player.setPoint(2, sf::Vector2f(player.getPoint(0).x, player.getPoint(0).y + 15.f));

    player_hitbox.setFillColor(sf::Color::Transparent);
    player_hitbox.setOutlineThickness(2.f);
    player_hitbox.setOutlineColor(sf::Color::Red);
    player_hitbox.setPoint(0, player.getPoint(0));
    player_hitbox.setPoint(1, player.getPoint(1));
    player_hitbox.setPoint(2, player.getPoint(2));
    // --------------------------------------------------------------------

    // score
    // --------------------------------------------------------------------
    sf::Text score_text("Score: " + std::to_string(score), font, 20);
    sf::Text high_score_text("High Score: " + std::to_string(high_score), font, 20);
    sf::Text energy_text("Energy", font, 20);

    score_text.setPosition(10.f, score_text.getLocalBounds().height / 2);
    high_score_text.setPosition(10.f, score_text.getLocalBounds().height * 2);
    energy_text.setPosition(555.f, score_text.getLocalBounds().height / 2 + 3.f);

    sf::RectangleShape energy_background(sf::Vector2f(140.f, 20.f));
    sf::RectangleShape energy_bar(sf::Vector2f(140.f, 20.f));

    energy_background.setPosition(650.f, score_text.getLocalBounds().height / 2 + 3.f);
    energy_background.setFillColor(sf::Color(50, 50, 50, 255));
    energy_background.setOutlineThickness(2.f);
    energy_background.setOutlineColor(sf::Color::White);

    energy_bar.setPosition(650.f, score_text.getLocalBounds().height / 2 + 3.f);
    energy_bar.setFillColor(sf::Color::Cyan);
    // --------------------------------------------------------------------

    // main
    // --------------------------------------------------------------------
    while(window.isOpen()) {
        sf::Event event;

        while(window.pollEvent(event)) {
            if(event.type == sf::Event::Closed) {
                window.close();
            }
        }

        // menu handling
        // --------------------------------------------------------------------
        if(current_state == MAIN_MENU) {
            menu_mode();

            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Enter)) {
                current_state = PLAYING;
            }
        }

        else if(current_state == PLAYING) {
            cooldown++;
            shot_time++;
            spawn_chance++;
            swap_count++;

            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Grave) && swap_count > swap_speed) {
                show_hitboxes = !show_hitboxes;
                swap_count = 0;
            }

            // ------------------------------------------------------------
            // Manual turning
            // ------------------------------------------------------------
            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
                rotate(player, 3, -1);
                rotate(player_hitbox, 3, -1);
            }

            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
                rotate(player, 3, 1);
                rotate(player_hitbox, 3, 1);
            }

            // ------------------------------------------------------------
            // Energy
            // ------------------------------------------------------------
            bool firing = sf::Keyboard::isKeyPressed(sf::Keyboard::Up);

            if(!firing) {
                energy += ENERGY_REGEN;

                if(energy > MAX_ENERGY) {
                    energy = MAX_ENERGY;
                }
            }

            // ------------------------------------------------------------
            // Shooting
            // ------------------------------------------------------------
            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Up) && (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) && shot_time >= interval && energy >= 25.f) {
                Projectile projectile(player, 2);
                projs.push_back(projectile);
                energy -= projectile.energy_cost;
                shot_time = -shot_time;
            }
            else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Up) && shot_time >= interval && energy >= 10.f) {
                Projectile projectile(player, 1);
                projs.push_back(projectile);
                energy -= projectile.energy_cost;
                shot_time = 0;
            }

            // ------------------------------------------------------------
            // Spawn asteroids
            // ------------------------------------------------------------
            if(randint(0, 100) == 50 && spawn_chance > spawn_rate) {
                belt.push_back(Asteroid(900.f, randint(0.f, 800.f)));
                spawn_chance = 0;
            }

            orientation %= 360;

            // ------------------------------------------------------------
            // Movement
            // ------------------------------------------------------------
            float radians = orientation * (M_PI / 180);
            float move_x = speed * cos(radians);
            float move_y = speed * sin(radians);

            player.move(move_x, move_y);
            player_hitbox.move(move_x, move_y);

            // ------------------------------------------------------------
            // Wall collision
            // ------------------------------------------------------------
            sf::FloatRect bounds = player.getGlobalBounds();

            bool hit_left = bounds.left <= 0.f;
            bool hit_right = bounds.left + bounds.width >= window.getSize().x;
            bool hit_top = bounds.top <= 0.f;
            bool hit_bottom = bounds.top + bounds.height >= window.getSize().y;

            if(hit_top) {
                float correction = -bounds.top;
                player.move(0.f, correction);
                player_hitbox.move(0.f, correction);
            }

            if(hit_bottom) {
                float correction = window.getSize().y - (bounds.top + bounds.height);
                player.move(0.f, correction);
                player_hitbox.move(0.f, correction);
            }

            if(hit_left) {
                float correction = -bounds.left;
                player.move(correction, 0.f);
                player_hitbox.move(correction, 0.f);
            }

            if(hit_right) {
                float correction = window.getSize().x - (bounds.left + bounds.width);
                player.move(correction, 0.f);
                player_hitbox.move(correction, 0.f);
            }

            // ------------------------------------------------------------
            // Projectile movement
            // ------------------------------------------------------------
            for(Projectile &i : projs) {
                i.travel(speed);
            }

            // ------------------------------------------------------------
            // Trail
            // ------------------------------------------------------------
            if(cooldown > 5) {
                cooldown = 0;

                sf::CircleShape segment(7.5f);
                segment.setFillColor(sf::Color::Transparent);
                segment.setOutlineThickness(2.f);
                segment.setOutlineColor(sf::Color(0, 200, 255, 255));

                float offsetX = -25 * cos(radians);
                float offsetY = -25 * sin(radians);

                segment.setPosition(player.getPosition().x + center.x + offsetX, player.getPosition().y + center.y + offsetY);
                trail.push_back(segment);

                if(trail.size() > 10) {
                    trail.erase(trail.begin());
                }
            }

            window.clear();

            // ------------------------------------------------------------
            // Update player trail
            // ------------------------------------------------------------
            for(int i = 0; i < trail.size(); i++) {
                sf::Color trail_color = trail[i].getOutlineColor();
                sf::Uint8 new_alpha = trail_color.a > 4 ? trail_color.a - 4 : 0;

                trail[i].setOutlineColor(sf::Color(trail_color.r, trail_color.g, trail_color.b, new_alpha));
                window.draw(trail[i]);
            }

            // ------------------------------------------------------------
            // Update projectiles
            // ------------------------------------------------------------
            for(int i = 0; i < projs.size(); i++) {
                window.draw(projs[i]);

                if(show_hitboxes) {
                    window.draw(projs[i].hitbox);
                }

                sf::FloatRect bounds = projs[i].getGlobalBounds();

                if(bounds.left + bounds.width < 0 || bounds.top + bounds.height < 0 || bounds.left > window.getSize().x || bounds.top > window.getSize().y) {
                    projs.erase(projs.begin() + i);
                    i--;
                }
            }

            // ------------------------------------------------------------
            // Update asteroids
            // ------------------------------------------------------------
            for(int i = 0; i < belt.size(); i++) {
                belt[i].move(-belt[i].speed, 0.f);
                window.draw(belt[i]);

                if(show_hitboxes) {
                    window.draw(belt[i].hitbox);
                }

                if(check_collision(belt[i].hitbox, player_hitbox)) {
                    player_death();
                }

                for(int j = 0; j < projs.size(); j++) {
                    if(check_collision(belt[i].hitbox, projs[j].hitbox)) {
                        belt[i].durability -= projs[j].power;

                        display_explosion(
                            sf::Vector2f(
                                projs[j].getPosition().x, 
                                projs[j].getPosition().y
                            ), 
                            projs[j].getFillColor(), 
                            2.5f
                        );

                        projs.erase(projs.begin() + j);
                        j--;
                    }
                }

                if(belt[i].durability <= 0) {
                    // Asteroid destruction explosion
                    display_explosion(
                        sf::Vector2f(
                            belt[i].getGlobalBounds().left + belt[i].getGlobalBounds().width / 2.f, 
                            belt[i].getGlobalBounds().top + belt[i].getGlobalBounds().height / 2.f
                        ), 
                        belt[i].getFillColor(), 
                        belt[i].radius * 0.5f
                    );

                    belt.erase(belt.begin() + i);
                    i--;
                    continue;
                }

                sf::FloatRect bounds = belt[i].getGlobalBounds();

                if(bounds.left + bounds.width < 0) {
                    belt.erase(belt.begin() + i);
                    i--;
                    continue;
                }
            }

            // ------------------------------------------------------------
            // Explosion
            // ------------------------------------------------------------
            if(explosion_active) {
                float elapsed = explosion_clock.getElapsedTime().asSeconds();
                float progress = elapsed / EXPLOSION_DURATION;

                if(progress >= 1.f) {
                    explosion_active = false;
                }
                else {
                    float radius = explosion_start_radius + (EXPLOSION_MAX_RADIUS - explosion_start_radius) * progress;
                    explosion.setRadius(radius);
                    explosion.setOrigin(radius, radius);

                    sf::Uint8 alpha = static_cast<sf::Uint8>(255.f * (1.f - progress));
                    explosion.setOutlineColor(sf::Color(explosion_color.r, explosion_color.g, explosion_color.b, alpha));
                    window.draw(explosion);
                }
            }

            // ------------------------------------------------------------
            // Draw player
            // ------------------------------------------------------------
            window.draw(player);

            if(show_hitboxes) {
                window.draw(player_hitbox);
            }

            // ------------------------------------------------------------
            // Energy bar
            // ------------------------------------------------------------
            energy_bar.setSize(sf::Vector2f(140.f * (energy / MAX_ENERGY), 20.f));

            window.draw(score_text);
            window.draw(high_score_text);
            window.draw(energy_text);
            window.draw(energy_background);
            window.draw(energy_bar);

            // ------------------------------------------------------------
            // Update score
            // ------------------------------------------------------------
            interval_count++;

            if(interval_count == SCORE_INTERVAL) {
                score++;
                interval_count = 0;
            }

            score_text.setString("Score: " + std::to_string(score));
            high_score_text.setString("High Score: " + std::to_string(high_score));

            // ------------------------------------------------------------
            // Display
            // ------------------------------------------------------------
            window.display();
        }

        else if(current_state == GAME_OVER) {
            window.clear();
            game_over();
        }
    }

    return 0;
}