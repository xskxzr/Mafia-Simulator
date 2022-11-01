#import <algorithm>
#import <iostream>
#import <random>
#import <string>
#import <vector>

namespace ranges = std::ranges;

int random(int a, int b) 
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution dist{a, b - 1};
    return dist(gen);
}

int random(int b) 
{
    return random(0, b);
}

int random(int a, int b, int exclude) 
{
    auto r = random(a, b - 1);
    if (r >= exclude) return r + 1;
    return r;
}


struct Game {
    enum Result {win, lose, uncertain};

    struct Special_villager {
        std::string name;
        int priority;
        bool is_disclosed = false;
    };

    void disclose_special_villager(std::vector<Special_villager>::iterator iter)
    {
        iter->is_disclosed = true;
        certain_special_villagers.emplace_back(std::move(*iter));
        uncertain_special_villagers.erase(iter);
    }

    Result remove_certain_special_villager(std::vector<Special_villager>::const_iterator iter)
    {
        if (iter->name == "hunter") hunter_killed_or_exiled = true;
        if (iter->name == "seer") seer_alive = false;
        if (iter->name == "witch") witch_alive = false;
        certain_special_villagers.erase(iter);
        if (certain_special_villagers.empty() && uncertain_special_villagers.empty()) return Result::lose;
        return Result::uncertain;
    }

    Result remove_uncertain_special_villager(std::vector<Special_villager>::const_iterator iter, bool is_poisoned = false)
    {
        if (iter->name == "hunter" && !is_poisoned) hunter_killed_or_exiled = true;
        if (iter->name == "seer") seer_alive = false;
        if (iter->name == "witch") witch_alive = false;
        uncertain_special_villagers.erase(iter);
        if (uncertain_special_villagers.empty() && certain_special_villagers.empty()) return Result::lose;
        return Result::uncertain;
    }

    Result remove_certain_ordinary_villager()
    {
        if (--num_certain_ordinary_villagers == 0 && num_uncertain_ordinary_villagers == 0) return Result::lose;
        return Result::uncertain;
    }

    Result remove_uncertain_ordinary_villager()
    {
        if (--num_uncertain_ordinary_villagers == 0 && num_certain_ordinary_villagers == 0) return Result::lose;
        return Result::uncertain;
    }

    Result remove_certain_mafia()
    {
        if (--num_certain_mafias == 0 && num_uncertain_mafias == 0) return Result::win;
        return Result::uncertain;
    }

    Result remove_uncertain_mafia()
    {
        if (--num_uncertain_mafias == 0 && num_certain_mafias == 0) return Result::win;
        return Result::uncertain;
    }

    Result villagers_exile(int depth = 1)
    {
        if (num_certain_mafias > 0) return remove_certain_mafia();
        auto exile = random(std::ssize(uncertain_special_villagers) + num_uncertain_ordinary_villagers + num_uncertain_mafias);
        if (exile < std::ssize(uncertain_special_villagers)) {
            auto iter = uncertain_special_villagers.begin() + exile;
            if (depth == 1 || random(depth) == 0) {
                disclose_special_villager(iter);
                return villagers_exile(depth + 1);
            }
            if (iter->name == "idiot") {
                disclose_special_villager(iter);
                return Result::uncertain;
            }
            return remove_uncertain_special_villager(iter);
        }
        if (exile < std::ssize(uncertain_special_villagers) + num_uncertain_ordinary_villagers) return remove_uncertain_ordinary_villager();
        return remove_uncertain_mafia();
    }

    Result hunter_kill()
    {
        if (!hunter_killed_or_exiled) return Result::uncertain;
        hunter_killed_or_exiled = false;
        if (num_certain_mafias > 0) return remove_certain_mafia();
        auto killed = random(std::ssize(uncertain_special_villagers) + num_uncertain_ordinary_villagers + num_uncertain_mafias);
        if (killed < std::ssize(uncertain_special_villagers)) return remove_uncertain_special_villager(uncertain_special_villagers.begin() + killed);
        if (killed < std::ssize(uncertain_special_villagers) + num_uncertain_ordinary_villagers) return remove_uncertain_ordinary_villager();
        return remove_uncertain_mafia();
    }

    Result day()
    {
        auto result = hunter_kill();
        if (result != Result::uncertain) return result;

        if (num_day == 1 || seer_alive && num_day <= 3) {
            auto seen = random(std::ssize(uncertain_special_villagers) + num_uncertain_ordinary_villagers + num_uncertain_mafias);
            if (seen < std::ssize(uncertain_special_villagers)) {
                auto seen_iter = uncertain_special_villagers.begin() + seen;
                certain_special_villagers.emplace_back(std::move(*seen_iter));
                uncertain_special_villagers.erase(seen_iter);
            }
            else if (seen < std::ssize(uncertain_special_villagers) + num_uncertain_ordinary_villagers) {
                ++num_certain_ordinary_villagers;
                --num_uncertain_ordinary_villagers;
            }
            else {
                ++num_certain_mafias;
                --num_uncertain_mafias;
            }
        }

        result = villagers_exile();
        if (result != Result::uncertain) return result;
        return hunter_kill();
    }

    Result night()
    {
        Result result;
        int poisoned = -1;

        if (++num_day == 2 && witch_alive) return Result::uncertain;

        if (num_day == 3 && witch_alive) {
            poisoned = random(std::ssize(uncertain_special_villagers) + num_uncertain_ordinary_villagers + num_uncertain_mafias);
            if (poisoned < std::ssize(uncertain_special_villagers) && uncertain_special_villagers[poisoned].name == "witch") 
                poisoned = random(0, std::ssize(uncertain_special_villagers) + num_uncertain_ordinary_villagers + num_uncertain_mafias, poisoned);
        }

        if (certain_special_villagers.empty()) {
            if (num_certain_ordinary_villagers == 0) {
                auto killed = random(std::ssize(uncertain_special_villagers) + num_uncertain_ordinary_villagers);
                if (killed < std::ssize(uncertain_special_villagers)) result = remove_uncertain_special_villager(uncertain_special_villagers.begin() + killed);
                else result = remove_uncertain_ordinary_villager();
                if (poisoned == killed) {
                    poisoned = -1;
                    hunter_killed_or_exiled = false;
                }
                else if (poisoned > killed) --poisoned;
            }
            else result = remove_certain_ordinary_villager();
        }
        else {
            auto killed_iter = ranges::min_element(certain_special_villagers, 
                [](const Special_villager &l, const Special_villager &r) 
                {
                    return l.is_disclosed && (!r.is_disclosed || l.priority < r.priority);
                });

            if (killed_iter->is_disclosed) result = remove_certain_special_villager(killed_iter);
            else {
                auto killed = random(std::ssize(certain_special_villagers) + num_certain_ordinary_villagers);
                if (killed < std::ssize(certain_special_villagers)) result = remove_certain_special_villager(certain_special_villagers.begin() + killed);
                else result = remove_certain_ordinary_villager();
            }
        }

        if (result != Result::uncertain || poisoned == -1) return result;
        if (poisoned < std::ssize(uncertain_special_villagers)) return remove_uncertain_special_villager(uncertain_special_villagers.begin() + poisoned, true);
        if (poisoned < std::ssize(uncertain_special_villagers) + num_uncertain_ordinary_villagers) return remove_uncertain_ordinary_villager();
        return remove_uncertain_mafia();
    }

    std::vector<Special_villager> certain_special_villagers, uncertain_special_villagers{{"seer", 0}, {"witch", 1}, {"hunter", 3}, {"idiot", 2}};
    int num_day = 0, num_certain_ordinary_villagers = 0, num_uncertain_ordinary_villagers = 4, num_certain_mafias = 0, num_uncertain_mafias = 4;
    bool hunter_killed_or_exiled = false, seer_alive = true, witch_alive = true;
};

int main()
{
    
    int count = 0, test_num = 1;
    for (int i = 0; i != test_num; ++i) {
        Game game;
        Game::Result result;
        
        game.night();
        if (game.seer_alive) {
            auto iter = ranges::find(game.uncertain_special_villagers, "seer", [](const Game::Special_villager &l){return l.name;});
            game.disclose_special_villager(iter);
        }
        do {
            result = game.day();
            std::cout << "after day:" << game.certain_special_villagers.size() << game.uncertain_special_villagers.size() << game.num_certain_ordinary_villagers << game.num_uncertain_ordinary_villagers << game.num_certain_mafias << game.num_uncertain_mafias << result << std::endl;
            if (result != Game::Result::uncertain) break;
            result = game.night();
            std::cout << "after night:" << game.certain_special_villagers.size() << game.uncertain_special_villagers.size() << game.num_certain_ordinary_villagers << game.num_uncertain_ordinary_villagers << game.num_certain_mafias << game.num_uncertain_mafias << result << std::endl;
        } while (result == Game::Result::uncertain);
        if (result == Game::Result::win) ++count;
    }
    std::cout << "count:" << count;
}