#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <windows.h>
#include <wincrypt.h>

#define  PACK_MAX_VOLUME  75	// 背包最大体积
#define  PACK_MAX_WEIGHT  80    // 背包最大承重
#define  MAX_GENERATION   50    // 遗传的最大代数

// 获取一个随机整数0-65535
static unsigned int Rand()
{
	unsigned int rand;

	HCRYPTPROV hcryp;
	CryptAcquireContext(&hcryp, NULL, NULL, PROV_RSA_FULL, 0);
	CryptGenRandom(hcryp, 2, (PBYTE)&rand);
	CryptReleaseContext(hcryp, 0);

	return rand;
}

// 获取一个0-1之间的随机浮点数
static float Rand_Float()
{
	return (Rand() % 1000 / 1000.0);
}

// 物品定义
struct Item
{
	Item(int va, int vo, int we) : value(va), volume(vo), weight(we) {}

	int value;		//价值
	int volume;     //体积
	int weight;     //重量
};

// 遗传 - 个体定义    每个个体就是一种选择方案
struct Entity
{
	Entity(int goods_num) : fitness(0), sum_volume(0), sum_weight(0), generation_id(0) { InitGene(goods_num); }
	Entity() : fitness(0), sum_volume(0), sum_weight(0), generation_id(0) { gene.clear(); }

	Entity &operator=(const Entity &rv)
	{
		fitness = rv.fitness;
		sum_volume = rv.sum_volume;
		sum_weight = rv.sum_weight;
		generation_id = rv.generation_id;
		gene = rv.gene;
		return *this;
	}

	void InitGene(int goods_num) { gene.assign(goods_num, 0); }  // In all items, 1 is selected,0 means no selection.

	int         fitness;        // Value
	int			sum_volume;		// Volume
	int         sum_weight;     // Weights
	int         generation_id;  // The optimal individual records this value
	std::vector<int>  gene;     // gene
};


// 遗传算法定义   Genetic Algorithm
class GA
{
public:
	static bool EntitySort(const Entity &one, const Entity &two) { return (one.fitness < two.fitness); }

	static float TotalFitness(const std::vector<Entity> &group)
	{
		float sum = 0.0;
		for (size_t i = 0; i < group.size(); i++)		//size_t is defined by unsigned int
		{
			sum += group[i].fitness;
		}
		return sum;
	}

public:
	GA(int gn, float cr, float vr, int goods_n) : m_group_num(gn), m_cross_rate(cr),
		m_varia_rate(vr), m_goods_num(goods_n)
	{
		Init_Goods();
	}

	void Run()	// main
	{
		Init_Group();

		for (int i = 0; i < MAX_GENERATION; i++)
		{
			CalcFitness();

			RecordOptimalEntity(i);

			Select();

			Cross();

			if (i % 5 == 0 && i != 0)
			{
				Variation();
			}
		}
	}

	// 显示最优个体
	void PrintOptimal()
	{
		std::cout << "\nThe optimal solution is in " << m_best_entity.generation_id << " generation." << std::endl;
		std::cout << "Total value:" << m_best_entity.fitness << "  Total volume:" << m_best_entity.sum_volume << "  Total weight:" << m_best_entity.sum_weight << std::endl;
		std::cout << "Final Solution: ";
		for (size_t i = 0; i < m_best_entity.gene.size(); i++)
		{
			std::cout << m_best_entity.gene[i] << " ";
		}
		std::cout << std::endl;
		//std::cout << "21709155    " << "梅海洋    " << "计算机科学与技术学院" << "\n";
	}

private:
	// Initialize the collection of items
	void Init_Goods()
	{
		std::cout << "Information of products.\nItem   Value    Volume    Weight：\n";
		std::ifstream infile("1.csv");
		if(!infile.good())
		{
		    std::cout << "File does not exist!" << "\n";
		    exit(0);
		}
		while (infile.good())
		{
			for (int i = 0; i < m_goods_num; i++)
			{
				std::string va,vo,we;
				getline(infile, va, ',');
				getline(infile, vo, ',');
				getline(infile, we, ',');
				m_goods.push_back( Item( atoi(va.c_str()), atoi(vo.c_str()), atoi(we.c_str()) ) );
			}
		}
		for (int i = 0; i < m_goods_num; i++)
		{
			std::cout << "物品" << i+1 << "  价值：" << m_goods[i].value << "  体积：" << m_goods[i].volume << "  重量：" << m_goods[i].weight << "\n";
		}
	}

	// Initialize the population
	void Init_Group()
	{
		m_best_entity.InitGene(m_goods_num);	// At begin, best gene is all zero.

		int volume = 0, weight = 0, count = 0;

		for (int i = 0; i < m_group_num; i++)
		{
			m_group.push_back(Entity(m_goods_num));
			Entity &en = m_group.back();

			// 产生一个0.5-1.0倍的承重值
			float vir_capacity_volume = ((Rand() % 50 + 50) / 100.0) * PACK_MAX_VOLUME;
			float vir_capacity_weight = ((Rand() % 50 + 50) / 100.0) * PACK_MAX_WEIGHT;

			// 初始化该个体基因中的每一位
			volume = 0;
			weight = 0;
			count = 0;
			while ((volume <= vir_capacity_volume) && (weight <= vir_capacity_weight) )
			{
				int idx = Rand() % m_goods_num;

				// 连续3次都随机到基因中的为1的染色体，则该个体初始化结束
				if (count == 3) { break; }
				if (en.gene[idx]) { count++; continue; }

				en.gene[idx] = 1;
				volume += m_goods[idx].volume;
				weight += m_goods[idx].weight;
			}
		}
	}

	// 适应度计算
	void CalcFitness()
	{
		for (size_t i = 0; i < m_group.size(); i++)
		{
			Entity &en = m_group[i];
			int val = 0, vol = 0, wei = 0 ;
			for (size_t j = 0; j < en.gene.size(); j++)
			{
				if (en.gene[j])
				{
					val += m_goods[j].value;
					vol += m_goods[j].volume;
					wei += m_goods[j].weight;
				}
			}

			if ( vol > PACK_MAX_VOLUME || wei > PACK_MAX_WEIGHT) { en.fitness = 0; continue; }

			en.fitness = val;
			en.sum_volume = vol;
			en.sum_weight = wei;
		}
	}

	// 记录最优个体
	void RecordOptimalEntity(int gid)
	{
		std::stable_sort(m_group.begin(), m_group.end(), GA::EntitySort);	//Sort from small to large
		Entity &en = m_group.back();
		if (en.fitness > m_best_entity.fitness)
		{
			m_best_entity = en;
			m_best_entity.generation_id = gid;
		}

		std::cout << "Group Number: " << m_group.size() << "  Generation: " << gid << "th  Value: " << en.fitness << "  Volume: " << en.sum_volume
			<< "  Weight: " << en.sum_weight << "\nSolution: ";
		for (size_t i = 0; i < en.gene.size(); i++)
		{
			std::cout << en.gene[i] << ",";
		}
		std::cout << std::endl;
	}

	// 选择
	void Select()
	{
		std::vector<Entity>  new_group;

		std::stable_sort(m_group.begin(), m_group.end(), GA::EntitySort);

		int src_group_num = m_group.size();

		// 保留群体中最优的前10%
		int reserve_num = (int)(src_group_num * 0.1);
		for (int i = 0; i < reserve_num; i++)
		{
			new_group.push_back(m_group.back());
			m_group.pop_back();
		}

		// 计算剩余个体的累积概率
		std::vector<float>  selected_rate;
		float sum_fitness = GA::TotalFitness(m_group);
		selected_rate.push_back(m_group[0].fitness / sum_fitness);
		for (size_t i = 1; i < m_group.size(); i++)
		{
			float cur_rate = selected_rate.back() + (m_group[i].fitness / sum_fitness);
			selected_rate.push_back(cur_rate);
		}

		// 利用轮赌法选择剩下的40%
		int left_num = (int)(src_group_num * 0.4);
		for (int i = 0; i < left_num; i++)
		{
			float rand_rate = Rand_Float();	// 0--1
			for (size_t idx = 0; idx < selected_rate.size(); idx++)
			{
				if (rand_rate <= selected_rate[idx])
				{
					new_group.push_back(m_group[idx]);
					break;
				}
			}
		}

		// 新群体赋值
		m_group.clear();
		m_group = new_group;
	}

	// 是否交叉
	bool IsCross() { return (Rand_Float() <= m_cross_rate); }

	// 交叉
	void Cross()
	{
		size_t src_group_num = m_group.size();

		for (size_t i = 0; i < src_group_num - 1; i += 2)
		{
			Entity en1 = m_group[i];
			Entity en2 = m_group[i + 1];

			for (size_t j = 0; j < en1.gene.size(); j++)
			{
				if (IsCross())
				{
					int tmp = en1.gene[j];
					en1.gene[j] = en2.gene[j];
					en2.gene[j] = tmp;
				}
			}

			m_group.push_back(en1);
			m_group.push_back(en2);
		}
	}

	// 是否变异
	bool IsVariation() { return (Rand_Float() <= m_varia_rate); }

	// 变异
	void Variation()
	{
		for (size_t i = 0; i < m_group.size(); i++)
		{
			if (IsVariation())
			{
				Entity &en = m_group[i];
				for (size_t j = 0; j < en.gene.size(); j++)
				{
					if (IsVariation())
					{
						en.gene[j] = (en.gene[j] ? 0 : 1);
					}
				}
			}
		}
	}

private:
	int					m_group_num;    // 群体数量
	float               m_cross_rate;   // 交叉概率
	float               m_varia_rate;   // 变异概率
	int					m_goods_num;    // 物品总数
	Entity              m_best_entity;  // 最优个体

	std::vector<Entity>       m_group;    // 遗传 - 群体
	std::vector<Item>		  m_goods;    // 总的物品集合
};


int main()
{
	GA ga(400, 0.5, 0.5, 32);
	ga.Run();
	ga.PrintOptimal();

	return 0;
}
