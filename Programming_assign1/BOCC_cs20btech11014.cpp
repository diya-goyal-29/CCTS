#include "BOCC_cs20btech11014.h"

Transaction *BOCC::begin_trans()
{
	global_lock.lock();
	Transaction *T = new Transaction(currTransNumber, LiveSet.size());
	for (auto t : LiveSet)
	{
		t->reference_count++;
		t->locLiveSet.insert(T);
	}
	T->locLiveSet = LiveSet;
	LiveSet.insert(T);
	currTransNumber++;
	global_lock.unlock();
	return T;
}

void BOCC::read(Transaction *T, int index, DataItem *array, int local_data)
{
	if (LiveSet.contains(T))
	{
		global_lock.lock_shared();
		local_data = array[index].value;
		array[index].lock.lock_shared();
		T->ReadSet.insert(&array[index]);
		array[index].ReadList.insert(T);
		array[index].lock.unlock();
		global_lock.unlock();
	}
}

void BOCC::write(Transaction *T, int value, DataItem *index)
{
	T->LocalWrite[index] = value;
	T->WriteSet.insert(index);
}

char BOCC::tryCommit(Transaction *T)
{
	global_lock.lock();
	for (auto d : T->ReadSet)
	{
		d->lock.lock();
		for (auto r : d->WriteList)
		{
			if (r->getStatus() == 'c' && T->Start < r->End)
			{
				T->setStatus('a');
				T->End = clock();
			}
		}
	}
	if (T->getStatus() == 'l')
	{
		for (auto d : T->WriteSet)
		{
			if (!T->ReadSet.contains(d))
			{
				d->lock.lock();
			}
			d->value = T->LocalWrite[d];
			d->WriteList.insert(T);
			d->lock.unlock();
		}
		for (auto e : T->ReadSet)
		{
			if (!T->WriteSet.contains(e))
			{
				e->lock.unlock();
			}
		}
		T->setStatus('c');
		CommittedList.insert(T);
		T->End = clock();
	}
	for (auto t : T->locLiveSet)
	{
		t->reference_count--;
		t->locLiveSet.erase(T);
	}
	LiveSet.erase(T);
	if (T->getStatus() == 'a')
	{
		for (auto d : T->ReadSet)
		{
			d->ReadList.erase(T);
			d->lock.unlock();
		}
		delete T;
		global_lock.unlock();
		return 'a';
	}

	global_lock.unlock();
	return 'c';
}

void BOCC::Garbage_Collection(Transaction *T)
{
	if (T->reference_count == 0)
	{
		for (auto d : T->ReadSet)
		{
			d->lock.lock_shared();
			d->ReadList.erase(T);
			d->lock.unlock();
		}
		for (auto d : T->WriteSet)
		{
			d->lock.lock_shared();
			d->WriteList.erase(T);
			d->lock.unlock();
		}
		CommittedList.erase(T);
		delete T;
	}
}
