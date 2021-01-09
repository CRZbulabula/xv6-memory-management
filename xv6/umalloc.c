#include "types.h"
#include "stat.h"
#include "user.h"
#include "param.h"

// Memory allocator by Kernighan and Ritchie,
// The C programming Language, 2nd ed.  Section 8.7.

typedef long Align;

union header {
  struct {
    union header *ptr;
    uint size;
  } s;
  Align x;
};

typedef union header Header;

static Header base;
static Header *freep,*firp;

#define first_fit
// #define next_fit
// #define best_fit
// #define worst_fit
void
free(void *ap)
{
  Header *bp, *p;

  bp = (Header*)ap - 1;
  printf(1, "free\nbp_size=%d\n",bp->s.size);
  for(p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
    if(p >= p->s.ptr && (bp > p || bp < p->s.ptr))
      break;
  if(bp + bp->s.size == p->s.ptr){
    //printf(1,"1..\n");
    bp->s.size += p->s.ptr->s.size;
    bp->s.ptr = p->s.ptr->s.ptr;
  } else
  {
    //printf(1,"2..\n");
    bp->s.ptr = p->s.ptr;
  }
  if(p + p->s.size == bp){
    //printf(1,"3..\n");
    p->s.size += bp->s.size;
    p->s.ptr = bp->s.ptr;
  } else
  {
    //printf(1,"4..\n");
    p->s.ptr = bp;
  }
    
  //printf(1,"p=%d p.size=%d p.ptr=%d\n",p,p->s.size,p->s.ptr);
  if(firp)
    for(p=firp->s.ptr;p!=firp;p=p->s.ptr)
    {
      printf(1,"%d\n",p->s.size);
    }
  freep = p;
  
  //printf(1,"freep_ed=%d\n",freep);
}

static Header*
morecore(uint nu)
{
  char *p;
  Header *hp;

  if(nu < 4096)
    nu = 4096;
  p = sbrk(nu * sizeof(Header));
  if(p == (char*)-1)
    return 0;
  hp = (Header*)p;
  hp->s.size = nu;
  //printf(1,"hp=%d,hp->size=%d\n",hp,hp->s.size);
  free((void*)(hp + 1));
  firp = freep->s.ptr;
  //printf(1,"firp=%d\n");
  return freep;
}


void*
malloc(uint nbytes)
{
  Header *p, *prevp;
  uint nunits;
  printf(1,"malloc\n");
  nunits = (nbytes + sizeof(Header) - 1)/sizeof(Header) + 1;
  printf(1,"%d nunits=%d\n",nbytes,nunits);
  if((prevp = freep) == 0){
    base.s.ptr = freep = prevp = &base;
    //printf(1, "&base=%d",&base);
    base.s.size = 0;
  }
  #ifdef next_fit
  for(p = prevp->s.ptr; ; prevp = p, p = p->s.ptr)
  {
    if(p->s.size >= nunits)
    {
      printf(1,"%d\n",p->s.size);
      if(p->s.size == nunits)
        prevp->s.ptr = p->s.ptr;
      else 
      {
        p->s.size -= nunits;
        p += p->s.size;
        p->s.size = nunits;
      }
      freep = prevp;
      
      return (void*)(p + 1);
    }
    if(p == freep)
    {
      if((p = morecore(nunits)) == 0)
      {
        return 0;
      }
    }
  }
  #endif

  #ifdef first_fit
  for(p = prevp->s.ptr; ; prevp = p, p = p->s.ptr)
  {
    printf(1,"%d\n",p->s.size);
    if(p->s.size >= nunits)
    {
      if(p->s.size == nunits)
        prevp->s.ptr = p->s.ptr;
      else 
      {
        p->s.size -= nunits;
        p += p->s.size;
        p->s.size = nunits;
      }
      return (void*)(p + 1);
    }
    if(p == freep)
    {
      if((p = morecore(nunits)) == 0)
      {
        return 0;
      }
    }
  }
  #endif

  #ifdef best_fit
  Header *nw;
  int flag=0;
  uint min_size=10000000;
  for(p = prevp->s.ptr; ; prevp = p, p = p->s.ptr)
  {
    if(p->s.size == nunits)
    {
      prevp->s.ptr = p->s.ptr;
      return (void*)(p + 1);
    }
    if(p->s.size > nunits)
    {
      flag=1;
      if(min_size > p->s.size-nunits)
      {
        min_size=p->s.size-nunits;
        nw=p;
      }
    }
    if(p == freep)
    {
      if (flag != 0)
      {
        nw->s.size -= nunits;
        nw += nw->s.size;
        nw->s.size = nunits;
        return (void*)(nw + 1);
      }
      else if((p = morecore(nunits)) == 0)
      {
        return 0;
      }
    }
  }
  #endif

  #ifdef worst_fit
  Header *nw,*pre_nw;
  int flag=0;
  int max_size=-1;
  for(p = prevp->s.ptr; ; prevp = p, p = p->s.ptr)
  {
    //printf(1,"p->s.size=%d\n",p->s.size);
    //if(firp) printf(1,"firp->ptr->size=%d\n",firp->s.ptr->s.size); else printf(1,"firp->ptr=0\n");
    if(p->s.size >= nunits)
    {
      flag=1;
      if(firp)
      {
        if (p!=firp)
          if(max_size < (int)(p->s.size-nunits))
          {
            max_size=p->s.size-nunits;
            nw=p;
            pre_nw=prevp;
          }
      }
      
    }
    
    if(p == freep)
    {
      //printf(1,"flag=%d max_size=%d \n",flag,max_size);
      if (flag != 0)
      {
        if(max_size==-1)
        {
          Header *midp;
          if(!firp) return 0;
          midp=firp;
          midp->s.size -= nunits;
          midp += midp->s.size;
          midp->s.size = nunits;
          return (void*)(midp + 1);
        }
        else 
        {
          if(max_size == 0)
          {
            pre_nw->s.ptr = nw->s.ptr; 
          }
          else
          {
            nw->s.size -= nunits;
            nw += nw->s.size;
            nw->s.size = nunits;
          }
          return (void*)(nw + 1);
        }
        
        
      }
      else if((p = morecore(nunits)) == 0)
      {
        return 0;
      }
    }
  }
  #endif
}

int total_piece()
{
  Header *p;
  int ans=0;
  for(p = firp->s.ptr; p!=firp ; p = p->s.ptr)
  {
    printf(1,"size_piece=%d\n",p->s.size);
    if (p->s.size!=0)
      ans += p->s.size;
  }
  return ans;
}
